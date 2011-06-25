/*
 *  drivers/mtd/nand/lf1000_ecc.c
 *
 *  Andrey Yurovsky <andrey@cozybit.com>
 *  Robert Dowling <rdowling@leapfrog.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <mach/platform.h>
#include <mach/common.h>
#include <mach/nand.h>
#include <asm/io.h>
#include <asm/sizes.h>

/* control registers */
#define NAND_BASE	IO_ADDRESS(LF1000_MCU_S_BASE)

#if defined CPU_LF1000 && defined CONFIG_MTD_NAND_LF1000_HWECC

/**
 * lf1000_nand_read_page_syndrome - [REPLACABLE] hardware ecc syndrom based page read
 * @mtd:	mtd info structure
 * @chip:	nand chip info structure
 * @buf:	buffer to store read data
 *
 * The hw generator calculates the error syndrome automatically. Therefor
 * we need a special oob layout and handling.
 */
int lf1000_nand_read_page_syndrome(struct mtd_info *mtd, struct nand_chip *chip,
				   uint8_t *buf)
{
	int i,j, eccsize = chip->ecc.size;
	int eccbytes = chip->ecc.bytes;
	int eccsteps = chip->ecc.steps;
	uint8_t *p = buf;
	uint8_t *oob = chip->oob_poi;
	uint8_t oob_whole[NAND_MAX_OOBSIZE];
	uint32_t *eccpos = chip->ecc.layout->eccpos;

	int is_large_block = mtd->writesize > 512;
	printk (KERN_DEBUG "!!! lf1000_nand_read_page_syndrome() is_large_block=%d\n", is_large_block);

	// Position over to oob, read whole oob
	// chip->read_buf(mtd, p, eccsize);
	// chip->read_buf(mtd, oob, mtd->oobsize);
	memset (oob_whole, 0xaa, mtd->oobsize);
	if (is_large_block)
		chip->cmdfunc(mtd, NAND_CMD_RNDOUT, mtd->writesize, -1);
	else
		chip->cmdfunc(mtd, NAND_CMD_READOOB, 0, -1);
	chip->read_buf(mtd, oob_whole, mtd->oobsize);

	if (0)
	{ char buf[80]; int x;
		for (i=0; i<mtd->oobsize; i+=16)
		{
			for (x=0, j=0; j<16; j++)
				x+=sprintf (buf+x, " %02x", oob_whole[i+j]);
			printk (KERN_DEBUG "%02x: %s\n", i, buf);
		}
	}

	// Position back to main area, read whole area a piece at a time
	if (is_large_block)
		chip->cmdfunc(mtd, NAND_CMD_RNDOUT, 0, -1);
	else
		chip->cmdfunc(mtd, NAND_CMD_READ0, 0, -1);
	for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {
		int stat;

		/* Set up oob for ecc.hwctl */
		for (j=0; j<eccbytes; j++)
		{
			oob[j] = oob_whole[eccpos[i+j]];
		}

		chip->ecc.hwctl(mtd, NAND_ECC_READ);	// Copy oob[0-6] oob into hardware regs
		chip->read_buf(mtd, p, eccsize);	// Read main data into p
		printk (KERN_DEBUG " read_buf=%02x,%02x,%02x,%02x  oob=%02x,%02x,%02x,%02x\n", p[0], p[1], p[2], p[3], oob[0], oob[1], oob[2], oob[3]);
		stat = chip->ecc.correct(mtd, p, oob, NULL);

		if (stat < 0)
			mtd->ecc_stats.failed++;
		else
			mtd->ecc_stats.corrected += stat;
	}

	// Position back to end of oob
        chip->cmdfunc(mtd, NAND_CMD_RNDOUT, mtd->writesize+mtd->oobsize, -1);

	/* Calculate remaining oob bytes
	i = mtd->oobsize - (oob - chip->oob_poi);
	if (i)
		chip->read_buf(mtd, oob, i);

	*/
	return 0;
}

/**
 * lf1000_nand_write_page_syndrome - [REPLACABLE] hardware ecc syndrom based page write
 * @mtd:	mtd info structure
 * @chip:	nand chip info structure
 * @buf:	data buffer
 *
 * The hw generator calculates the error syndrome automatically. Therefor
 * we need a special oob layout and handling.
 */
void lf1000_nand_write_page_syndrome(struct mtd_info *mtd,
				    struct nand_chip *chip, const uint8_t *buf)
{
	int i, j, eccsize = chip->ecc.size;
	int eccbytes = chip->ecc.bytes;
	int eccsteps = chip->ecc.steps;
	const uint8_t *p = buf;
	uint8_t *oob = chip->oob_poi;
	uint8_t oob_whole[NAND_MAX_OOBSIZE];
	uint32_t *eccpos = chip->ecc.layout->eccpos;

	printk (KERN_DEBUG "!!! lf1000_nand_write_page_syndrome() eccsize=%d\n", eccsize);

	memset (oob_whole, 0xff, mtd->oobsize);
	for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {

		u32 s4, s5, s6, s7;

		chip->ecc.hwctl(mtd, NAND_ECC_WRITE);
		// s4 = foo9 ();
		chip->write_buf(mtd, p, eccsize);
		// s5 = foo9 ();
		// chip->write_buf(mtd, p, eccsize/2);
		// s6 = foo9 ();

		/*
		if (chip->ecc.prepad) {
			chip->write_buf(mtd, oob, chip->ecc.prepad);
			oob += chip->ecc.prepad;
		}
		*/
		chip->ecc.calculate(mtd, p, oob);
		// s7 = foo9 ();
		for (j=0; j<eccbytes; j++)
		{
			oob_whole[eccpos[i+j]] = oob[j];
		}

		// chip->write_buf(mtd, oob, eccbytes);
		// bar = foo9 ();

		/*
		printk (KERN_DEBUG " in nand_base, s4=%d s5=%d s6=%d s7=%d foo=%d bar=%d\n", s4, s5, s6, s7, foo, bar);
		printk (KERN_DEBUG " write_buf=%02x,%02x,%02x,%02x  oob=%02x,%02x,%02x,%02x\n", p[0], p[1], p[2], p[3], oob[0], oob[1], oob[2], oob[3]);
		*/
		
		/*
		if (chip->ecc.postpad) {
			chip->write_buf(mtd, oob, chip->ecc.postpad);
			oob += chip->ecc.postpad;
		}
		*/
	}

	if (0)
	{ char buf[80]; int x;
		for (i=0; i<mtd->oobsize; i+=16)
		{
			for (x=0, j=0; j<16; j++)
				x+=sprintf (buf+x, " %02x", oob_whole[i+j]);
			printk (KERN_DEBUG "%02x: %s\n", i, buf);
		}
	}

	/* Calculate remaining oob bytes */
	i = mtd->oobsize; // - (oob - chip->oob_poi);
	if (i)
		chip->write_buf(mtd, oob_whole, i);
}


int foo9 ()
{
	return IS_CLR(readl(NAND_BASE+NFECCSTATUS), NFECCENCDONE);
}

/*
 * The LF1000 hardware ECC is always enabled so no initialization is needed 
 * here.
 *
 * nand_read_page_syndrome() uses this function as follows:
 * - calls with NAND_ECC_READ, we need to grab the ECC and put it into ORGECC
 * - reads ecc.size bytes
 * - calls with NAND_ECC_READSYN, we need to wait for the decoder to finish
 *
 * nand_write_page_syndrome() uses this function as follows:
 * - calls with NAND_ECC_WRITE, we don't have to do anything
 * - writes ecc.size bytes
 * - (calls lf1000_calculate_ecc(), where we can wait for encoder to finish)
 */ 
void lf1000_enable_hwecc(struct mtd_info *mtd, int mode)
{
	u32 x, s7, s5, s3, s1, val32;
	struct nand_chip *chip = mtd->priv;
	uint8_t *oob = chip->oob_poi;

	switch(mode) {
	case NAND_ECC_READ:
		/* TODO: 'seek' to the right spot in the OOB, retrieve the ECC data
		 * 	 and place into the ORGECC registers and then reset the column
		 * 	 address for the normal write that will take place. */
		/* XXX: for now, we will just zero out the ORGECC */
		printk (KERN_DEBUG "!!! lf1000_enable_hwecc(NAND_ECC_READ)\n");

		/* Reset the HW ECC block (without changing the interrupts pending)
		 * Note: This was commented out of the VTK demo code, but it seems to help make writes consistent
		 */
		x = readl(NAND_BASE+NFCONTROL);
		x &= ~((1UL << ECCRST) | (1UL << IRQPEND));
		x |= (1UL << ECCRST);
		writel (x, NAND_BASE+NFCONTROL);
		
		val32 = oob[0] 
			+ (oob[1] << 8)
			+ (oob[2] << 16)
			+ (oob[3] << 24);
		printk (KERN_DEBUG "  val32 Low =%08x\n", val32);
		writel(val32, NAND_BASE+NFORGECCL); 

		val32 = oob[4] 
			+ (oob[5] << 8)
			+ (oob[6] << 16);
		printk (KERN_DEBUG "  val32 High=%08x\n", val32);
		writel(val32, NAND_BASE+NFORGECCH); 

		/* WORK IN PROGRESS... Fake out ECC for known patterns for debugging */
		/* Stuff fake ECC code for now */
		/* writel(0xd59e1ce9, NAND_BASE+NFORGECCL);	* for all ff data */
		/* writel(0x000b92b9, NAND_BASE+NFORGECCH);	* for all ff data */
		/*   writel(0xcdfb7ec1, NAND_BASE+NFORGECCL);	 * for 1234567890 then all ff data */
		/*   writel(0x000a0189, NAND_BASE+NFORGECCH);	 * for 1234567890 then all ff data */
		/* writel(0x2dd2bac0, NAND_BASE+NFORGECCL);	* for 1334567890 then all ff data */
		/* writel(0x000a1fd2, NAND_BASE+NFORGECCH);	* for 1334567890 then all ff data */
		break;

	case NAND_ECC_WRITE:
		s1 = foo9 ();
		printk (KERN_DEBUG "!!! lf1000_enable_hwecc(NAND_ECC_WRITE)\n");
		// MagicEyes data sheet says reading ECC resets ENCDONE bit
		val32 = readl(NAND_BASE+NFECCH) & 0xffffff;
		val32 = readl(NAND_BASE+NFECCL);
		s3 = foo9 ();
		/* Reading NAND_BASE+NFCNT does not help...
		 *  x = readw(NAND_BASE+NFCNT); printk (KERN_DEBUG "    NFCNT = %08x\n", x); */
		/* Reset the HW ECC block (without changing the interrupts pending)
		 * Note: This was commented out of the VTK demo code, but it seems to help make writes consistent */
		
		x = readl(NAND_BASE+NFCONTROL);
		x &= ~((1UL << ECCRST) | (1UL << IRQPEND));
		x |= (1UL << ECCRST);
		writel (x, NAND_BASE+NFCONTROL);

		s5 = foo9 ();
		printk (KERN_DEBUG "!!! lf1000_enable_hwecc(NAND_ECC_WRITE) s1=%d s3=%d s5=%d\n", s1,s3,s5);
		break;

	case NAND_ECC_READSYN:
		printk (KERN_DEBUG "!!! lf1000_enable_hwecc(NAND_ECC_READSYN)\n");
		/* wait for ECC decoder to be done */
		while(IS_CLR(readl(NAND_BASE+NFECCSTATUS), NFECCDECDONE))
			;

		/* Pull out the 4 syndrome words */
		x = readl(NAND_BASE+NFSYNDRONE75);
		s7 = (x >> SYNDROM7) & 0x1fff;
		s5 = (x >> SYNDROM5) & 0x1fff;
		x = readl(NAND_BASE+NFSYNDRONE31);
		s3 = (x >> SYNDROM3) & 0x1fff;
		s1 = (x >> SYNDROM1) & 0x1fff;
		printk (KERN_DEBUG "!!!! s1=%04x s3=%04x s5=%04x s7=%04x\n", s1, s3, s5, s7);
		break;
	}
}

/* 
 * Retrieve the ECC.  Since we have Syndrome mode, this function is called only
 * on writes.  Here we need to make sure that the ECC Encoder is done encoding,
 * and then retrieve the encoded ECC so that it can be written to the OOB area.
 */

/* FYI: *dat is a 256 byte block of raw data */
int lf1000_calculate_ecc(struct mtd_info *mtd, const uint8_t *dat,
				uint8_t *ecc_code)
{
	int i = 0;
	u32 ecch, eccl;

	/* u32 x = readl(NAND_BASE+NFCNT);
	   printk (KERN_DEBUG "    NFCNT = %08x\n", x); */

	/* wait for ECC encoder to be done */
	while(IS_CLR(readl(NAND_BASE+NFECCSTATUS), NFECCENCDONE))	/* Was readw */
		;

	eccl = readl(NAND_BASE+NFECCL);
	ecch = readl(NAND_BASE+NFECCH) & 0xffffff;

	/* retrieve encoded ECC, to be written to the OOB area */
	/* Order of ecc_code 0=low[lsb]..3=low[msb],4=high[lsb]..7=high[msb] */
	for(; i < 4; i++)
	{
		ecc_code[i] = eccl & 0xff;
		eccl >>= 8;
	}
	for(; i < 7; i++)
	{
		ecc_code[i] = ecch & 0xff;
		ecch >>= 8;
	}
	printk (KERN_DEBUG "!!!! lf1000_calculate_ecc (on write) '%10.10s' %02x %02x %02x %02x %02x %02x %02x\n", (char *)dat,
		ecc_code[0], ecc_code[1], ecc_code[2], ecc_code[3], ecc_code[4], ecc_code[5], ecc_code[6]);

	// eccl = readl(NAND_BASE+NFORGECCL);
	// ecch = readl(NAND_BASE+NFORGECCH) & 0xffffff;
	eccl = readl(NAND_BASE+NFCNT);
	printk (KERN_DEBUG "!!!! lf1000_calculate_ecc cnt=%08x\n", eccl);


	return 0;
}

/* This function is called on a read, after retrieving data and decoding the
 * ECC.  The hardware will generate an error bit and the Odd Syndrome.
 * Return: Number of corrected errors or -1 for failure */
int lf1000_correct_ecc(struct mtd_info *mtd, uint8_t *dat,
				uint8_t *read_ecc, uint8_t *calc_ecc)
{
	u32 x;
	u16 s7, s5, s3, s1;
	int lf1000_GetErrorLocation (int *pLocation, u16 s1, u16 s3, u16 s5, u16 s7);
	int loc[5], numErrors, numFixed=0;
	struct nand_chip *chip = mtd->priv;
	int eccbytes = chip->ecc.bytes;
	int eccsize = chip->ecc.size;

	printk (KERN_DEBUG "!!! lf1000_correct_ecc\n");

	/* wait for ECC decoder to be done */
	while(IS_CLR(readl(NAND_BASE+NFECCSTATUS), NFECCDECDONE))
		;
	/* Check if hardware detected any error at all */
	if(IS_SET(readl(NAND_BASE+NFECCSTATUS), NFCHECKERROR)) {
		/* Erorr detected */
		printk("!!!! mtd-lf1000: ECC reports an error\n");

		/* TODO: Here we need to use the Odd Syndrome to determine the
		 *       error position, and then try to fix it.
		 */
		
		/* Pull out the 4 syndrome words */
		x = readl(NAND_BASE+NFSYNDRONE75);
		s7 = (x >> SYNDROM7) & 0x1fff;
		s5 = (x >> SYNDROM5) & 0x1fff;
		x = readl(NAND_BASE+NFSYNDRONE31);
		s3 = (x >> SYNDROM3) & 0x1fff;
		s1 = (x >> SYNDROM1) & 0x1fff;
		loc[0] = loc[1] = loc[2] = loc[3] = loc[4] = -1;
		numErrors= lf1000_GetErrorLocation (&loc, s1, s3, s5, s7);
		printk (KERN_DEBUG "!!!! lf1000_correct_ecc: s1=%04x s3=%04x s5=%04x s7=%04x numErrors=%d Loc=%d,%d,%d,%d,%d\n", 
			s1, s3, s5, s7, numErrors, loc[0], loc[1], loc[2], loc[3], loc[4]);

		if (numErrors < 0)  /* uncorrectable errors */
		{
			/* Check if the section and its ECC area are completely
			 * erased.  If they are, don't report an uncorrectable
			 * error.
			 * TODO: FIXME: Decide if this is what we ought to check
			 *				that the entire page and OOB are FF before
			 * saying that the "uncorrectable" error is due to an
			 * erased and unwritten page.
			 */
			int eccSubIndex;
			int dataIndex;
			for (eccSubIndex = 0; eccSubIndex < eccbytes; ++eccSubIndex)
			{
				if (read_ecc[eccSubIndex] != 0xFF)
					break;
			}
			/*  check the data only if all ECC bytes are 0xFF */
			if (eccSubIndex >= eccbytes)
			{
				for (dataIndex = 0; dataIndex < eccsize; ++dataIndex)
				{
					if (dat[dataIndex] != 0xFF)
						break;
				}
				if (dataIndex >= eccsize)
				{
					printk(KERN_INFO "Totally erased.  Ignoring ECC\n");
					return 0;
				}
			}
			/* If an uncorrectable error, increment mtd->ecc_stats.failed
			 * and return immediately with an error indication
			 */
			if ((eccSubIndex < eccbytes) || (dataIndex < eccsize))
			{
				mtd->ecc_stats.failed++;
				printk(KERN_INFO "Uncorrectable ECC error\n");

				// Lets print everything out now
				{
					int i, j, b;
					char chuf[128];
					for (b=0, i=0; i<eccbytes; i++)
						b+=sprintf (chuf+b, "%02x ", read_ecc[i]);
					printk ("ECC: %s\n", chuf);
					for (i=0; i<512; i+=16)
					{
						for (b=0, j=0; j<16; j++)
							b+=sprintf (chuf+b, "%02x ", dat[i+j]);
						printk ("%04x: %s\n", i, chuf);
					}
				}
				// Fail and return IO error
				return -EIO;
			}
		}
		/* If there is one or more correctable errors
		 * ('numErrors' is the number of correctable errors)
		 */
		else if (numErrors > 0)
		{
			int errorOffset;
			int errorMask;
			int j;
			for (j = 0; j < numErrors; ++j)
			{
				int k;
				for (k = 0; k < j; ++k)
				{
					if (loc[j] == loc[k])

					{
						break;	// break if we've already seen this error
					}
				}
				if (k >= j)
				{
					// We've never before seen this bit flipped
					errorOffset = loc[j] >> 3;
					errorMask   = 1 << (loc[j] & 7);
					/* We see errorOffset >= 512 when an ECC bit is in error
					 * Don't bother correcting ECC bit errors, because the
					 * bit positions might be beyond the end of the buffer.
					 */
					if (errorOffset < eccsize)
					{
						*(dat + errorOffset) ^= errorMask;
					}
					printk(KERN_INFO "  (%d, %d, x%02x)\n", loc[j], errorOffset, errorMask);
					numFixed++;
				}
			}
		}
	}
	return numFixed;
}

/******************************************************************************
 * Lifted from Magic Eyes ./vtk/test/NAND_Flash/mes_nand.c
 *
//  Copyright (C) 2007 MagicEyes Digital Co., All Rights Reserved
//  MagicEyes Digital Co. Proprietary & Confidential
//
//	MAGICEYES INFORMS THAT THIS CODE AND INFORMATION IS PROVIDED "AS IS" BASE
//  AND WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING
//  BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS
//  FOR A PARTICULAR PURPOSE.
 *
 * MES_NAND_GetErrorLocation( int *pLocation )
 ******************************************************************************/

extern const short BCH_AlphaToTable[8192];
extern const short BCH_IndexOfTable[8192];
extern const unsigned int BCH_TGRTable[52][2];


//------------------------------------------------------------------------------
// Derived from Magic Eyes's
// int MES_NAND_GetErrorLocation( int *pLocation )
//
int lf1000_GetErrorLocation (int *pLocation, u16 s1, u16 s3, u16 s5, u16 s7)
{
	register int i, j, elp_sum ;
	int count;
	int r;			// Iteration steps
	int Delta; 		// Discrepancy value
	int elp[8+1][8+2]; 	// Error locator polynomial (ELP)
	int L[8+1];		// Degree of ELP
	int B[8+1][8+2];	// Scratch polynomial
	//	int root[8+1];	// Roots of ELP
	int reg[8+1];		// Register state
	int	s[8];

	s[0] = s1;
	s[2] = s3;
	s[4] = s5;
	s[6] = s7;

	// Even syndrome = (Odd syndrome) ** 2
	for( i=1,j=0 ; i<8 ; i+=2, j++ )
	{
		if( s[j] == 0 )		s[i] = 0;
		else				s[i] = BCH_AlphaToTable[(2 * BCH_IndexOfTable[s[j]]) % 8191];
	}

	// Initialization of elp, B and register
	for( i=0 ; i<=8 ; i++ )
	{
		L[i] = 0 ;
		for( j=0 ; j<=8 ; j++ )
		{
			elp[i][j] = 0 ;
			B[i][j] = 0 ;
		}
	}

	for( i=0 ; i<=4 ; i++ )
	{
		//		root[i] = 0 ;
		reg[i] = 0 ;
	}

	elp[1][0] = 1 ;
	elp[1][1] = s[0] ;

	L[1] = 1 ;
	if( s[0] != 0 )
		B[1][0] = BCH_AlphaToTable[(8191 - BCH_IndexOfTable[s[0]]) % 8191];
	else
		B[1][0] = 0;

	for( r=3 ; r<=8-1 ; r=r+2 )
	{
		// Compute discrepancy
		Delta = s[r-1] ;
		for( i=1 ; i<=L[r-2] ; i++ )
		{
			if( (s[r-i-1] != 0) && (elp[r-2][i] != 0) )
				Delta ^= BCH_AlphaToTable[(BCH_IndexOfTable[s[r-i-1]] + BCH_IndexOfTable[elp[r-2][i]]) % 8191];
		}

		if( Delta == 0 )
		{
			L[r] = L[r-2] ;
			for( i=0 ; i<=L[r-2] ; i++ )
			{
				elp[r][i] = elp[r-2][i];
				B[r][i+2] = B[r-2][i] ;
			}
		}
		else
		{
			// Form new error locator polynomial
			for( i=0 ; i<=L[r-2] ; i++ )
			{
				elp[r][i] = elp[r-2][i] ;
			}

			for( i=0 ; i<=L[r-2] ; i++ )
			{
				if( B[r-2][i] != 0 )
					elp[r][i+2] ^= BCH_AlphaToTable[(BCH_IndexOfTable[Delta] + BCH_IndexOfTable[B[r-2][i]]) % 8191];
			}

			// Form new scratch polynomial and register length
			if( 2 * L[r-2] >= r )
			{
				L[r] = L[r-2] ;
				for( i=0 ; i<=L[r-2] ; i++ )
				{
					B[r][i+2] = B[r-2][i];
				}
			}
			else
			{
				L[r] = r - L[r-2];
				for( i=0 ; i<=L[r-2] ; i++ )
				{
					if( elp[r-2][i] != 0 )
						B[r][i] = BCH_AlphaToTable[(BCH_IndexOfTable[elp[r-2][i]] + 8191 - BCH_IndexOfTable[Delta]) % 8191];
					else
						B[r][i] = 0;
				}
			}
		}
	}

	if( L[8-1] > 4 )
	{
		//return L[8-1];
		return -1;
	}
	else
	{
		// Chien's search to find roots of the error location polynomial
		// Ref: L&C pp.216, Fig.6.1
		for( i=1 ; i<=L[8-1] ; i++ )
			reg[i] = elp[8-1][i];

		count = 0;
		for( i=1 ; i<=8191 ; i++ )
		{
			elp_sum = 1;
			for( j=1 ; j<=L[8-1] ; j++ )
			{
				if( reg[j] != 0 )
				{
					reg[j] = BCH_AlphaToTable[(BCH_IndexOfTable[reg[j]] + j) % 8191] ;
					elp_sum ^= reg[j] ;
				}
			}

			if( !elp_sum )		// store root and error location number indices
			{
				//				root[count] = i;

				// Convert error location from systematic form to storage form
				pLocation[count] = 8191 - i;

				if (pLocation[count] >= 52)
				{
					// Data Bit Error
					pLocation[count] = pLocation[count] - 52;
					pLocation[count] = 4095 - pLocation[count];
				}
				else
				{
					// ECC Error
					pLocation[count] = pLocation[count] + 4096;
				}

				if( pLocation[count] < 0 )	return -1;
				/*
				  if( i <= 8191 - 52 )	pLocation[count] = 4095 - (8191 - 52 - i);
				  else					pLocation[count] = 4095 - (8191 + 4096 - i);
				*/

				printk (KERN_DEBUG "lf1000_GetErrorLocation: count=%d, location=%d, count++\n", count, pLocation[count]);
				count++;
			}
		}

		if( count == L[8-1] )	// Number of roots = degree of elp hence <= 4 errors
		{
			return L[8-1];
		}
		else	// Number of roots != degree of ELP => >4 errors and cannot solve
		{
			return -1;
		}
	}
}

#endif /* CPU_LF1000 && CONFIG_MTD_NAND_LF1000_HWECC */
