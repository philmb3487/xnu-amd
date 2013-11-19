/*
 * Copyright (c) 2005-2007 Apple Inc. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 * 
 * Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */
/*
 * @OSF_COPYRIGHT@
 */

/*
 *	File:		i386/tsc.c
 *	Purpose:	Initializes the TSC and the various conversion
 *			factors needed by other parts of the system.
 */

#include <platforms.h>

#include <mach/mach_types.h>

#include <kern/cpu_data.h>
#include <kern/cpu_number.h>
#include <kern/clock.h>
#include <kern/host_notify.h>
#include <kern/macro_help.h>
#include <kern/misc_protos.h>
#include <kern/spl.h>
#include <kern/assert.h>
#include <mach/vm_prot.h>
#include <vm/pmap.h>
#include <vm/vm_kern.h>		/* for kernel_map */
#include <architecture/i386/pio.h>
#include <i386/machine_cpu.h>
#include <i386/cpuid.h>
#include <i386/mp.h>
#include <i386/machine_routines.h>
#include <i386/proc_reg.h>
#include <i386/tsc.h>
#include <i386/misc_protos.h>
#include <pexpert/pexpert.h>
#include <machine/limits.h>
#include <machine/commpage.h>
#include <sys/kdebug.h>
#include <pexpert/device_tree.h>

uint64_t	busFCvtt2n = 0;
uint64_t	busFCvtn2t = 0;
uint64_t	tscFreq = 0;
uint64_t	tscFCvtt2n = 0;
uint64_t	tscFCvtn2t = 0;
uint64_t	tscGranularity = 0;
uint64_t	bus2tsc = 0;
uint64_t	busFreq = 0;
uint32_t	kTscPanicOn = 0;
uint32_t	flex_ratio = 0;
uint32_t	flex_ratio_min = 0;
uint32_t	flex_ratio_max = 0;


#define bit(n)		(1ULL << (n))
#define bitmask(h,l)	((bit(h)|(bit(h)-1)) & ~(bit(l)-1))
#define bitfield(x,h,l)	(((x) & bitmask(h,l)) >> l)

/* Decimal powers: */
#define kilo (1000ULL)
#define Mega (kilo * kilo)
#define Giga (kilo * Mega)
#define Tera (kilo * Giga)
#define Peta (kilo * Tera)

/* mercurysquad: The following enum specifies one of the bus ratio calc paths to take */
typedef enum {
	BUSRATIO_BOOTFLAG,
	BUSRATIO_ATHLON,
	BUSRATIO_EFI,
	BUSRATIO_PHENOM_SHANGHAI,
	BUSRATIO_INTEL_MSR,
	BUSRATIO_AUTODETECT,
	BUSRATIO_PENTIUM4_MSR, // P4 model 2+ have an MSR too
	BUSRATIO_TIMER
} busratio_path_t;

static const char* busRatioPathNames[] = {
	"Boot-time argument",
	"AMD Athlon",
	"Pentium 4 (via EFI)",
	"AMD Phenom",
	"Intel / Apple",
	"Autodetect",
	"Pentium 4 (via MSR)",
	"Time the TSC"
};

static const char	FSB_Frequency_prop[] = "FSBFrequency";
static const char  FSB_CPUFrequency_prop[] = "CPUFrequency";
/*
 * This routine extracts the bus frequency in Hz from the device tree.
 */
static uint64_t
EFI_FSB_frequency(void)
{
	uint64_t	frequency = 0;
	DTEntry		entry;
	void		*value;
	unsigned int	size;

	if (DTLookupEntry(0, "/efi/platform", &entry) != kSuccess) {
		kprintf("EFI_FSB_frequency: didn't find /efi/platform\n");
		return 0;
	}
	if (DTGetProperty(entry,FSB_Frequency_prop,&value,&size) != kSuccess) {
		kprintf("EFI_FSB_frequency: property %s not found\n",
			FSB_Frequency_prop);
		return 0;
	}
	if (size == sizeof(uint64_t)) {
		frequency = *(uint64_t *) value;
		kprintf("EFI_FSB_frequency: read %s value: %llu\n",
			FSB_Frequency_prop, frequency);
		if (!(90*Mega < frequency && frequency < 10*Giga)) {
			kprintf("EFI_FSB_frequency: value out of range\n");
			frequency = 0;
		}
	} else {
		kprintf("EFI_FSB_frequency: unexpected size %d\n", size);
	}
	return frequency;
}
/* mercurysquad:
 * This routine extracts the cpu frequency from the efi device tree
 * The value should be set by a custom EFI bootloader (only needed on CPUs which
 * don't report the bus ratio in one of the MSRs.)
 */
static uint64_t
EFI_CPU_Frequency(void)
{
	uint64_t	frequency = 0;
	DTEntry		entry;
	void		*value;
	unsigned int	size;
	
	if (DTLookupEntry(0, "/efi/platform", &entry) != kSuccess) {
		kprintf("EFI_CPU_Frequency: didn't find /efi/platform\n");
		return 0;
	}
	if (DTGetProperty(entry,FSB_CPUFrequency_prop,&value,&size) != kSuccess) {
		kprintf("EFI_CPU_Frequency: property %s not found\n",
			FSB_Frequency_prop);
		return 0;
	}
	if (size == sizeof(uint64_t)) {
		frequency = *(uint64_t *) value;
		kprintf("EFI_CPU_Frequency: read %s value: %llu\n",
			FSB_Frequency_prop, frequency);
		if (!(10*Mega < frequency && frequency < 50*Giga)) {
			kprintf("EFI_Fake_MSR: value out of range\n");
			frequency = 0;
		}
	} else {
		kprintf("EFI_CPU_Frequency: unexpected size %d\n", size);
	}
	return frequency;
}

/*
 * Convert the cpu frequency info into a 'fake' MSR198h in Intel format
 */
static uint64_t
getFakeMSR(uint64_t frequency, uint64_t bFreq) {
	uint64_t fakeMSR = 0ull;
	uint64_t multi = 0;
	
	if (frequency == 0 || bFreq == 0)
		return 0;
	
	multi = frequency / (bFreq / 1000); // = multi*1000
	// divide by 1000, rounding up if it was x.75 or more
	// Example: 12900 will get rounded to 13150/1000 = 13
	//          but 12480 will be 12730/1000 = 12
	fakeMSR = (multi + 250) / 1000;
	fakeMSR <<= 40; // push multiplier into bits 44 to 40
	
	// If fractional part was within (0.25, 0.75), set N/2
	if ((multi % 1000 > 250) && (multi % 1000 < 750))
		fakeMSR |= (1ull << 46);

	return fakeMSR;
}

int ForceAmdCpu = 0;

/* Handy functions to check what platform we're on */
boolean_t IsAmdCPU(void) {
	if (ForceAmdCpu) return TRUE;
	
	uint32_t ourcpuid[4];
	do_cpuid(0, ourcpuid);
	if (ourcpuid[ebx] == 0x68747541 &&
	    ourcpuid[ecx] == 0x444D4163 &&
	    ourcpuid[edx] == 0x69746E65)
		return TRUE;
	else
		return FALSE;
};

boolean_t IsIntelCPU(void) {
	return !IsAmdCPU(); // dirty hack
}


/*
 * Initialize the various conversion factors needed by code referencing
 * the TSC.
 */
void
tsc_init(void)
{
	boolean_t	N_by_2_bus_ratio = FALSE;

	if (cpuid_vmm_present()) {
		kprintf("VMM vendor %u TSC frequency %u KHz bus frequency %u KHz\n",
				cpuid_vmm_info()->cpuid_vmm_family,
				cpuid_vmm_info()->cpuid_vmm_tsc_frequency,
				cpuid_vmm_info()->cpuid_vmm_bus_frequency);

		if (cpuid_vmm_info()->cpuid_vmm_tsc_frequency &&
			cpuid_vmm_info()->cpuid_vmm_bus_frequency) {

			busFreq = (uint64_t)cpuid_vmm_info()->cpuid_vmm_bus_frequency * kilo;
			busFCvtt2n = ((1 * Giga) << 32) / busFreq;
			busFCvtn2t = 0xFFFFFFFFFFFFFFFFULL / busFCvtt2n;
			
			tscFreq = (uint64_t)cpuid_vmm_info()->cpuid_vmm_tsc_frequency * kilo;
			tscFCvtt2n = ((1 * Giga) << 32) / tscFreq;
			tscFCvtn2t = 0xFFFFFFFFFFFFFFFFULL / tscFCvtt2n;
			
			tscGranularity = tscFreq / busFreq;
			
			bus2tsc = tmrCvt(busFCvtt2n, tscFCvtn2t);

			return;
		}
	}

	/*
	 * Get the FSB frequency and conversion factors from EFI.
	 */
	busFreq = EFI_FSB_frequency();

	switch (cpuid_cpufamily()) {
	case CPUFAMILY_INTEL_HASWELL:
	case CPUFAMILY_INTEL_IVYBRIDGE:
	case CPUFAMILY_INTEL_SANDYBRIDGE:
	case CPUFAMILY_INTEL_WESTMERE:
	case CPUFAMILY_INTEL_NEHALEM: {
		uint64_t msr_flex_ratio;
		uint64_t msr_platform_info;

		/* See if FLEX_RATIO is being used */
		msr_flex_ratio = rdmsr64(MSR_FLEX_RATIO);
		msr_platform_info = rdmsr64(MSR_PLATFORM_INFO);
		flex_ratio_min = (uint32_t)bitfield(msr_platform_info, 47, 40);
		flex_ratio_max = (uint32_t)bitfield(msr_platform_info, 15, 8);
		/* No BIOS-programed flex ratio. Use hardware max as default */
		tscGranularity = flex_ratio_max;
		if (msr_flex_ratio & bit(16)) {
		 	/* Flex Enabled: Use this MSR if less than max */
			flex_ratio = (uint32_t)bitfield(msr_flex_ratio, 15, 8);
			if (flex_ratio < flex_ratio_max)
				tscGranularity = flex_ratio;
		}

		/* If EFI isn't configured correctly, use a constant 
		 * value. See 6036811.
		 */
		if (busFreq == 0)
		    busFreq = BASE_NHM_CLOCK_SOURCE;

            }
		break;
	default: {
	/*
 	 * mercurysquad: The bus ratio is crucial to setting the proper rtc increment.
 	 * There are several methods so we first check any bootlfags. If none is specified, we choose
 	 * based on the CPU type.
  	 */
 	uint64_t cpuFreq = 0, prfsts = 0, boot_arg = 0;
 	busratio_path_t busRatioPath = BUSRATIO_AUTODETECT;
 	
 	if (PE_parse_boot_argn("busratiopath", &boot_arg, sizeof(boot_arg)))
 		busRatioPath = (busratio_path_t) boot_arg;
 	else
 		busRatioPath = BUSRATIO_AUTODETECT;
	
 	if (PE_parse_boot_argn("busratio", &tscGranularity, sizeof(tscGranularity)))
 		busRatioPath = BUSRATIO_BOOTFLAG;
 	
 	if (busRatioPath == BUSRATIO_AUTODETECT) {
 		/* This happens if no bootflag above was specified.
 		 * We'll choose based on CPU type */
 		switch (cpuid_info()->cpuid_family) {
 			case CPU_FAMILY_PENTIUM_4:
 				/* This could be AMD Athlon or Intel P4 as both have family Fh */
 				if (IsAmdCPU())
 					busRatioPath = BUSRATIO_ATHLON;
 				else if (cpuid_info()->cpuid_model < 2 )
 					/* These models don't implement proper MSR 198h or 2Ch */
 					busRatioPath = BUSRATIO_TIMER;
 				else if (cpuid_info()->cpuid_model == 2)
 					/* This model has an MSR we can use */
 					busRatioPath = BUSRATIO_PENTIUM4_MSR;
 				else /* 3 or higher */
 					/* Other models should implement MSR 198h */
 					busRatioPath = BUSRATIO_INTEL_MSR;
 				break;
 			case CPU_FAMILY_PENTIUM_M:
 				if (cpuid_info()->cpuid_model >= 0xD)
 					/* Pentium M or Core and above can use Apple method*/
 					busRatioPath = BUSRATIO_INTEL_MSR;
 				else
 					/* Other Pentium class CPU, use safest option */
 					busRatioPath = BUSRATIO_TIMER;
 				break;
 			case CPU_FAMILY_AMD_PHENOM:
 			case CPU_FAMILY_AMD_SHANGHAI:
 				/* These have almost the same method, with a minor difference */
 				busRatioPath = BUSRATIO_PHENOM_SHANGHAI;
 				break;
 			default:
 				/* Fall back to safest method */
 				busRatioPath = BUSRATIO_TIMER;
 		};
 	}
 	
 	/* 
 	 * Now that we have elected a bus ratio path, we can proceed to calculate it.
 	 */
 	printf("rtclock_init: Taking bus ratio path %d (%s)\n",
 	       busRatioPath, busRatioPathNames[busRatioPath]);
 	switch (busRatioPath) {
 		case BUSRATIO_BOOTFLAG:
 			/* tscGranularity was already set. However, check for N/2. N/2 is specified by
 			 * giving a busratio of 10 times what it is (so last digit is 5). We set a cutoff
 			 * of 30 before deciding it's n/2. TODO: find a better way */
 			if (tscGranularity == 0) tscGranularity = 1; // avoid div by zero
 			N_by_2_bus_ratio = (tscGranularity > 30) && ((tscGranularity % 10) != 0);
 			if (N_by_2_bus_ratio) tscGranularity /= 10; /* Scale it back to normal */
 			break;
#ifndef __i386__ //AnV: in case of x86_64 boot default for busratio timer to EFI value
		case BUSRATIO_TIMER:
#endif
 		case BUSRATIO_EFI:
 			/* This uses the CPU frequency exported into EFI by the bootloader */
 			cpuFreq = EFI_CPU_Frequency();
 			prfsts  = getFakeMSR(cpuFreq, busFreq);
			tscGranularity = (uint32_t)bitfield(prfsts, 44, 40);
 			N_by_2_bus_ratio = prfsts & bit(46);
 			break;
 		case BUSRATIO_INTEL_MSR:
 			/* This will read the performance status MSR on intel systems (Apple method) */
 			prfsts = rdmsr64(IA32_PERF_STS);
 			tscGranularity	= (uint32_t)bitfield(prfsts, 44, 40);
 			N_by_2_bus_ratio= prfsts & bit(46);
 			break;
 		case BUSRATIO_ATHLON:
 			/* Athlons specify the bus ratio directly in an MSR using a simple formula */
 			prfsts		= rdmsr64(AMD_PERF_STS);
 			tscGranularity  = 4 + bitfield(prfsts, 5, 1);
 			N_by_2_bus_ratio= prfsts & bit(0); /* FIXME: This is experimental! */
 			break;
 		case BUSRATIO_PENTIUM4_MSR:
 			prfsts		= rdmsr64(0x2C); // TODO: Add to header
 			tscGranularity	= bitfield(prfsts, 31, 24);
 			break;
 		case BUSRATIO_PHENOM_SHANGHAI:
 			/* Phenoms and Shanghai processors have a different MSR to read the frequency
 			 * multiplier and divisor, from which the cpu frequency can be calculated.
 			 * This can then be used to construct the fake MSR. */
 			prfsts		= rdmsr64(AMD_COFVID_STS);
 			printf("rtclock_init: Phenom MSR 0x%x returned: 0x%llx\n", AMD_COFVID_STS, prfsts);
 			uint64_t cpuFid = bitfield(prfsts, 5, 0);
 			uint64_t cpuDid = bitfield(prfsts, 8, 6);
 			/* The base for Fid could be either 8 or 16 depending on the cpu family */
 			if (cpuid_info()->cpuid_family == CPU_FAMILY_AMD_PHENOM)
 				cpuFreq = (100 * Mega * (cpuFid + 0x10)) >> cpuDid;
 			else /* shanghai */
 				cpuFreq = (100 * Mega * (cpuFid + 0x08)) >> cpuDid;
 			prfsts = getFakeMSR(cpuFreq, busFreq);
 			tscGranularity = (uint32_t)bitfield(prfsts, 44, 40);
 			N_by_2_bus_ratio = prfsts & bit(46);
 			break;
#ifdef __i386__ //qoopz: no get_PIT2 for x86_64
 		case BUSRATIO_TIMER:
 			/* Fun fun fun. :-|  */
 			cpuFreq = timeRDTSC() * 20;
 			prfsts = getFakeMSR(cpuFreq, busFreq);
 			tscGranularity = (uint32_t)bitfield(prfsts, 44, 40);
 			N_by_2_bus_ratio = prfsts & bit(46);
 			break;
#endif
 		case BUSRATIO_AUTODETECT:
 		default:
 			kTscPanicOn = 1; /* see sanity check below */
 	};

#ifdef __i386__
 	/* Verify */
 	if (!PE_parse_boot_argn("-notscverify", &boot_arg, sizeof(boot_arg))) {
 		uint64_t realCpuFreq = timeRDTSC() * 20;
 		cpuFreq = tscGranularity * busFreq;
 		if (N_by_2_bus_ratio) cpuFreq += (busFreq / 2);
 		uint64_t difference = 0;
 		if (realCpuFreq > cpuFreq)
 			difference = realCpuFreq - cpuFreq;
 		else
 			difference = cpuFreq - realCpuFreq;
 		
 		if (difference >= 4*Mega) {
 			// Shouldn't have more than 4MHz difference. This is about 2-3% of most FSBs.
 			// Fall back to using measured speed and correct the busFreq
 			// Note that the tscGran was read from CPU so should be correct.
 			// Only on Phenom the tscGran is calculated by dividing by busFreq.
 			printf("TSC: Reported FSB: %4d.%04dMHz, ", (uint32_t)(busFreq / Mega), (uint32_t)(busFreq % Mega));
 			if (N_by_2_bus_ratio)
 				busFreq = (realCpuFreq * 2) / (1 + 2*tscGranularity);
 			else
 				busFreq = realCpuFreq / tscGranularity;
 			printf("corrected FSB: %4d.%04dMHz\n", (uint32_t)(busFreq / Mega), (uint32_t)(busFreq % Mega));
 			// Reset the busCvt factors
 			busFCvtt2n = ((1 * Giga) << 32) / busFreq;
 			busFCvtn2t = 0xFFFFFFFFFFFFFFFFULL / busFCvtt2n;
 			busFCvtInt = tmrCvt(1 * Peta, 0xFFFFFFFFFFFFFFFFULL / busFreq);
 			printf("TSC: Verification of clock speed failed. "
 			       "Fallback correction was performed. Please upgrade bootloader.\n");
 		} else {
 			printf("TSC: Verification of clock speed PASSED.\n");
 		}
 	}
#else
	printf("TSC: Verification of clock speed not available in x86_64.\n");
#endif
 	
 	/* Do a sanity check of the granularity */
 	if ((tscGranularity == 0) ||
 	    (tscGranularity > 30) ||
 	    (busFreq < 50*Mega) ||
 	    (busFreq > 1*Giga) ||
 	    /* The following is useful to force a panic to print diagnostic info */
 	    PE_parse_boot_argn("-tscpanic", &boot_arg, sizeof(boot_arg)))
 	{
 		printf("\n\n");
 		printf(" >>> The real-time clock was not properly initialized on your system!\n");
 		printf("     Contact Voodoo Software for further information.\n");
 		kTscPanicOn = 1; /* Later when the console is initialized, this will show up, and we'll halt */
 		if (tscGranularity == 0) tscGranularity = 1; /* to avoid divide-by-zero in the following few lines */
	}

	    }
		break;
	}

	if (busFreq != 0) {
		busFCvtt2n = ((1 * Giga) << 32) / busFreq;
		busFCvtn2t = 0xFFFFFFFFFFFFFFFFULL / busFCvtt2n;
	} else {
  		/* Instead of panicking, set a default FSB frequency */
  		busFreq = 133*Mega;
 		kprintf("rtclock_init: Setting fsb to %u MHz\n", (uint32_t) (busFreq/Mega));
	
	}

	kprintf(" BUS: Frequency = %6d.%06dMHz, "
		"cvtt2n = %08X.%08X, cvtn2t = %08X.%08X\n",
		(uint32_t)(busFreq / Mega),
		(uint32_t)(busFreq % Mega), 
		(uint32_t)(busFCvtt2n >> 32), (uint32_t)busFCvtt2n,
		(uint32_t)(busFCvtn2t >> 32), (uint32_t)busFCvtn2t);

	/*
	 * Get the TSC increment.  The TSC is incremented by this
	 * on every bus tick.  Calculate the TSC conversion factors
	 * to and from nano-seconds.
	 * The tsc granularity is also called the "bus ratio". If the N/2 bit
	 * is set this indicates the bus ration is 0.5 more than this - i.e.
	 * that the true bus ratio is (2*tscGranularity + 1)/2. If we cannot
	 * determine the TSC conversion, assume it ticks at the bus frequency.
	 */
	if (tscGranularity == 0)
		tscGranularity = 1;

	if (N_by_2_bus_ratio)
		tscFCvtt2n = busFCvtt2n * 2 / (1 + 2*tscGranularity);
	else
		tscFCvtt2n = busFCvtt2n / tscGranularity;

	tscFreq = ((1 * Giga)  << 32) / tscFCvtt2n;
	tscFCvtn2t = 0xFFFFFFFFFFFFFFFFULL / tscFCvtt2n;

	kprintf(" TSC: Frequency = %6d.%06dMHz, "
		"cvtt2n = %08X.%08X, cvtn2t = %08X.%08X, gran = %lld%s\n",
		(uint32_t)(tscFreq / Mega),
		(uint32_t)(tscFreq % Mega), 
		(uint32_t)(tscFCvtt2n >> 32), (uint32_t)tscFCvtt2n,
		(uint32_t)(tscFCvtn2t >> 32), (uint32_t)tscFCvtn2t,
		tscGranularity, N_by_2_bus_ratio ? " (N/2)" : "");

	/*
	 * Calculate conversion from BUS to TSC
	 */
	bus2tsc = tmrCvt(busFCvtt2n, tscFCvtn2t);
}

void
tsc_get_info(tscInfo_t *info)
{
	info->busFCvtt2n     = busFCvtt2n;
	info->busFCvtn2t     = busFCvtn2t;
	info->tscFreq        = tscFreq;
	info->tscFCvtt2n     = tscFCvtt2n;
	info->tscFCvtn2t     = tscFCvtn2t;
	info->tscGranularity = tscGranularity;
	info->bus2tsc        = bus2tsc;
	info->busFreq        = busFreq;
	info->flex_ratio     = flex_ratio;
	info->flex_ratio_min = flex_ratio_min;
	info->flex_ratio_max = flex_ratio_max;
}
