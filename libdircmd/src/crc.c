/**********************************************************************************************************************
 *                                                                                                                    *
 *  C R C . C                                                                                                         *
 *  =========                                                                                                         *
 *                                                                                                                    *
 *  This is free software; you can redistribute it and/or modify it under the terms of the GNU General Public         *
 *  License version 2 as published by the Free Software Foundation.  Note that I am not granting permission to        *
 *  redistribute or modify this under the terms of any later version of the General Public License.                   *
 *                                                                                                                    *
 *  This is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied        *
 *  warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more     *
 *  details.                                                                                                          *
 *                                                                                                                    *
 *  You should have received a copy of the GNU General Public License along with this program (in the file            *
 *  "COPYING"); if not, write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111,   *
 *  USA.                                                                                                              *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \file
 *  \brief Founction to calculate the crc of a file.
 */
#include <config.h>
#include <stdio.h>

#ifdef HAVE_OPENSSL_EVP_H
#include <openssl/evp.h>
#else
#ifdef HAVE_OPENSSL_MD5_H
#include <openssl/md5.h>
#endif
#ifdef HAVE_OPENSSL_SHA_H
#include <openssl/sha.h>
#endif
#endif

#ifdef HAVE_OPENSSL_EVP_H

/**********************************************************************************************************************
 *                                                                                                                    *
 *  M D  C H E C K  S U M                                                                                             *
 *  =====================                                                                                             *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief New way to do checksums, could add more.
 *  \param digestname What checksum is this.
 *  \param filename File to check sum.
 *  \param outBuffer Output buffer.
 *  \result 1 if all OK.
 */
int mdCheckSum (char *digestname, char *filename, unsigned char *outBuffer)
{
	EVP_MD_CTX *mdctx;
	const EVP_MD *md;
	int retn = 0;

	if (filename != NULL && outBuffer != NULL)
	{
		FILE *inFile = NULL;
		if ((inFile = fopen (filename, "rb")) != NULL)
		{
			unsigned int mdLen;
			char readBuff[4100];
			int readSize;
			
			OpenSSL_add_all_digests();
			if ((md = EVP_get_digestbyname(digestname)) != NULL)
			{
				mdctx = EVP_MD_CTX_create();
				EVP_DigestInit_ex(mdctx, md, NULL);

				retn = 1;
				while ((readSize = fread (readBuff, 1, 4096, inFile)) > 0)
				{
					EVP_DigestUpdate(mdctx, readBuff, readSize);
				}	
				fclose (inFile);
				EVP_DigestFinal_ex(mdctx, outBuffer, &mdLen);
				EVP_MD_CTX_destroy(mdctx);
			}
			EVP_cleanup();
		}
	}
	return retn;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  M D 5 F I L E                                                                                                     *
 *  =============                                                                                                     *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Call for MD5 check sums.
 *  \param filename File to check sum.
 *  \param md5Buffer Output buffer.
 *  \result 1 if all OK.
 */
int MD5File (char *filename, unsigned char *md5Buffer)
{
	return mdCheckSum ("md5", filename, md5Buffer);
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S H A 2 5 6 F I L E                                                                                               *
 *  ===================                                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Call for SHA256 check sums.
 *  \param filename File to check sum.
 *  \param shaBuffer Output buffer.
 *  \result 1 if all OK.
 */
int SHA256File (char *filename, unsigned char *shaBuffer)
{
	return mdCheckSum ("sha256", filename, shaBuffer);
}

#else
/**********************************************************************************************************************
 *                                                                                                                    *
 *  M D 5 F I L E                                                                                                     *
 *  =============                                                                                                     *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Create an MD5 for a file.
 *  \param filename Name of the file to MD5.
 *  \param md5Buffer Write the MD5 value here (must be 16 bytes long).
 *  \result 1 if file read.
 */
int MD5File (char *filename, unsigned char *md5Buffer)
{
	int retn = 0;
#ifdef HAVE_OPENSSL_MD5_H
	MD5_CTX md5c;

	if (filename != NULL && md5Buffer != NULL)
	{
		FILE *inFile;
		if ((inFile = fopen (filename, "rb")) != NULL)
		{
			int readSize;
			char readBuff[4100];
			
			MD5_Init (&md5c);
			retn = 1;
			while ((readSize = fread (readBuff, 1, 4096, inFile)) > 0)
			{
				MD5_Update (&md5c, readBuff, readSize);
			}
			fclose (inFile);
			MD5_Final (md5Buffer, &md5c);
		}
	}
#else
	*md5Buffer = 0;
#endif
	return retn;
}

/**********************************************************************************************************************
 *                                                                                                                    *
 *  S H A 2 5 6 F I L E                                                                                               *
 *  ===================                                                                                               *
 *                                                                                                                    *
 **********************************************************************************************************************/
/**
 *  \brief Create an SHA256 for a file.
 *  \param filename Name of the file to SHA256.
 *  \param shaBuffer Write the SHA256 value here (must be 32 bytes long).
 *  \result 1 if file read.
 */
int SHA256File (char *filename, unsigned char *shaBuffer)
{
	int retn = 0;
#ifdef HAVE_OPENSSL_SHA_H
	SHA256_CTX sha256c;

	if (filename != NULL && shaBuffer != NULL)
	{
		FILE *inFile;
		if ((inFile = fopen (filename, "rb")) != NULL)
		{
			int readSize;
			char readBuff[4100];
			
			SHA256_Init (&sha256c);
			retn = 1;
			while ((readSize = fread (readBuff, 1, 4096, inFile)) != 0)
			{
				SHA256_Update (&sha256c, readBuff, readSize);
			}
			fclose (inFile);
			SHA256_Final (shaBuffer, &sha256c);
		}
	}
#else
	*shaBuffer = 0;
#endif
	return retn;
}

#endif
