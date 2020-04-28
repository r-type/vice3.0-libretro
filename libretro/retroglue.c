#include "libretro-core.h"
#include "archdep.h"

#include "deps/libz/zlib.h"
#include "deps/libz/unzip.h"
#include "file/file_path.h"
void zip_uncompress(char *in, char *out, char *lastfile)
{
    unzFile uf = NULL;
    uf = unzOpen(in);

    uLong i;
    unz_global_info gi;
    int err;
    err = unzGetGlobalInfo (uf, &gi);

    const char* password = NULL;
    int size_buf = 8192;

    for (i = 0; i < gi.number_entry; i++)
    {
        char filename_inzip[256];
        char filename_withpath[512];
        filename_inzip[0] = '\0';
        filename_withpath[0] = '\0';
        char* filename_withoutpath;
        char* p;
        unz_file_info file_info;
        FILE *fout = NULL;
        void* buf;

        buf = (void*)malloc(size_buf);
        if (buf == NULL)
        {
            fprintf(stderr, "Unzip: Error allocating memory\n");
            return;
        }

        err = unzGetCurrentFileInfo(uf, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0);
        snprintf(filename_withpath, sizeof(filename_withpath), "%s%s%s", out, FSDEV_DIR_SEP_STR, filename_inzip);
        if ((dc_get_image_type(filename_inzip) == DC_IMAGE_TYPE_FLOPPY || dc_get_image_type(filename_inzip) == DC_IMAGE_TYPE_TAPE) && lastfile != NULL)
            snprintf(lastfile, RETRO_PATH_MAX, "%s", filename_inzip);
        p = filename_withoutpath = filename_inzip;
        while ((*p) != '\0')
        {
            if (((*p) == '/') || ((*p) == '\\'))
                filename_withoutpath = p+1;
            p++;
        }

        if ((*filename_withoutpath) == '\0')
        {
            fprintf(stdout, "Unzip mkdir:   %s\n", filename_withpath);
            path_mkdir(filename_withpath);
        }
        else
        {
            const char* write_filename;
            int skip = 0;

            write_filename = filename_withpath;

            err = unzOpenCurrentFilePassword(uf, password);
            if (err != UNZ_OK)
            {
                fprintf(stderr, "Unzip: Error %d with zipfile in unzOpenCurrentFilePassword: %s\n", err, write_filename);
            }

            if ((skip == 0) && (err == UNZ_OK))
            {
                fout = fopen(write_filename, "wb");
                if (fout == NULL)
                {
                    fprintf(stderr, "Unzip: Error opening %s\n", write_filename);
                }
            }

            if (fout != NULL)
            {
                fprintf(stdout, "Unzip extract: %s\n", write_filename);

                do
                {
                    err = unzReadCurrentFile(uf, buf, size_buf);
                    if (err < 0)
                    {
                        fprintf(stderr, "Unzip: Error %d with zipfile in unzReadCurrentFile\n", err);
                        break;
                    }
                    if (err > 0)
                    {
                        if (!fwrite(buf, err, 1, fout))
                        {
                            fprintf(stderr, "Unzip: Error in writing extracted file\n");
                            err = UNZ_ERRNO;
                            break;
                        }
                    }
                }
                while (err > 0);
                if (fout)
                    fclose(fout);
            }

            if (err == UNZ_OK)
            {
                err = unzCloseCurrentFile(uf);
                if (err != UNZ_OK)
                {
                    fprintf(stderr, "Unzip: Error %d with zipfile in unzCloseCurrentFile\n", err);
                }
            }
            else
                unzCloseCurrentFile(uf);
        }

        free(buf);

        if ((i+1) < gi.number_entry)
        {
            err = unzGoToNextFile(uf);
            if (err != UNZ_OK)
            {
                fprintf(stderr, "Unzip: Error %d with zipfile in unzGoToNextFile\n", err);
                break;
            }
        }
    }

    if (uf)
    {
        unzCloseCurrentFile(uf);
        unzClose(uf);
        uf = NULL;
    }
}

void remove_recurse(const char *path)
{
   struct dirent *dirp;
   char filename[RETRO_PATH_MAX];
   DIR *dir = opendir(path);
   if (dir == NULL)
      return;

   while ((dirp = readdir(dir)) != NULL)
   {
      if (dirp->d_name[0] == '.')
         continue;

      sprintf(filename, "%s%s%s", path, FSDEV_DIR_SEP_STR, dirp->d_name);
      fprintf(stdout, "Unzip clean: %s\n", filename);

      if (path_is_directory(filename))
         remove_recurse(filename);
      else
         remove(filename);
   }

   closedir(dir);
   archdep_rmdir(path);
}


/* NIBTOOLS */
typedef unsigned char BYTE;
typedef unsigned char __u_char;
#include "deps/nibtools/nibtools.h"

#include "deps/nibtools/gcr.c"
#include "deps/nibtools/prot.c"
#include "deps/nibtools/crc.c"
#include "deps/nibtools/bitshifter.c"

int write_dword(FILE * fd, unsigned int * buf, int num)
{
	int i;
	BYTE *tmpbuf;

	tmpbuf = malloc(num);

	for (i = 0; i < (num / 4); i++)
	{
		tmpbuf[i * 4] = buf[i] & 0xff;
		tmpbuf[i * 4 + 1] = (buf[i] >> 8) & 0xff;
		tmpbuf[i * 4 + 2] = (buf[i] >> 16) & 0xff;
		tmpbuf[i * 4 + 3] = (buf[i] >> 24) & 0xff;
	}

	if (fwrite(tmpbuf, num, 1, fd) < 1)
	{
		free(tmpbuf);
		return -1;
	}
	free(tmpbuf);
	return 0;
}

int compare_extension(unsigned char * filename, unsigned char * extension)
{
	char *dot;

	dot = strrchr((const char *)filename, '.');
	if (dot == NULL)
		return (0);

	for (++dot; *dot != '\0'; dot++, extension++)
		if (tolower(*dot) != tolower(*extension))
			return (0);

	if (*extension == '\0')
		return (1);
	else
		return (0);
}

int load_file(char *filename, BYTE *file_buffer)
{
	int size;
	FILE *fpin;

	printf("Loading \"%s\"...\n",filename);

	if ((fpin = fopen(filename, "rb")) == NULL)
	{
		fprintf(stderr, "Couldn't open input file %s!\n", filename);
		return 0;
	}

	fseek(fpin, 0, SEEK_END);
	size = ftell(fpin);
	rewind(fpin);

	if (fread(file_buffer, size, 1, fpin) != 1) {
			printf("unable to read file\n");
			return 0;
	}

	printf("Successfully loaded %d bytes.", size);
	fclose(fpin);
	return size;
}

int read_nib(BYTE *file_buffer, int file_buffer_size, BYTE *track_buffer, BYTE *track_density, size_t *track_length)
{
	int track, t_index=0, h_index=0;

	printf("\nParsing NIB data...\n");

	if (memcmp(file_buffer, "MNIB-1541-RAW", 13) != 0)
	{
		printf("Not valid NIB data!\n");
		return 0;
	}
	else
		printf("NIB file version %d\n", file_buffer[13]);

	while(file_buffer[0x10+h_index])
	{
		track = file_buffer[0x10+h_index];
		track_density[track] = (BYTE)(file_buffer[0x10 + h_index + 1]);
		track_density[track] %= BM_MATCH;  	 /* discard unused BM_MATCH mark */

		memcpy(track_buffer + (track * NIB_TRACK_LENGTH),
			file_buffer + (t_index * NIB_TRACK_LENGTH) + 0x100,
			NIB_TRACK_LENGTH);

		h_index+=2;
		t_index++;
	}
	printf("Successfully parsed NIB data for %d tracks\n", t_index);
	return 1;
}

int align_tracks(BYTE *track_buffer, BYTE *track_density, size_t *track_length, BYTE *track_alignment)
{
	int track;
	BYTE nibdata[NIB_TRACK_LENGTH];

	memset(nibdata, 0, sizeof(nibdata));
	printf("Aligning tracks...\n");

	for (track = start_track; track <= end_track; track ++)
	{
		if(verbose) printf("%4.1f: ",(float) track/2);
		memcpy(nibdata, track_buffer+(track*NIB_TRACK_LENGTH), NIB_TRACK_LENGTH);
		memset(track_buffer + (track * NIB_TRACK_LENGTH), 0x00, NIB_TRACK_LENGTH);

		/* Arnd's version */
		if (isTrackBitshifted(nibdata, NIB_TRACK_LENGTH))
		{
			printf("[bitshifted] ");
			align_bitshifted_track(nibdata, NIB_TRACK_LENGTH, NULL, NULL);
		}

		/* process track cycle */
		track_length[track] = extract_GCR_track(
			track_buffer + (track * NIB_TRACK_LENGTH),
			nibdata,
			&track_alignment[track],
			track/2,
			capacity_min[track_density[track]&3],
			capacity_max[track_density[track]&3]
		);

		/* output some specs */
		if(verbose)
		{
			if(track_density[track] & BM_NO_SYNC) printf("NOSYNC:");
			if(track_density[track] & BM_FF_TRACK) printf("KILLER:");
			printf("(%d:", track_density[track]&3);
			printf("%d) ", track_length[track]);
			printf("[align=%s]\n",alignments[track_alignment[track]]);
		}
	}
	return 1;
}

int write_g64(char *filename, BYTE *track_buffer, BYTE *track_density, size_t *track_length)
{
	/* writes contents of buffers into G64 file, with header and density information */

	/* when writing a G64 file, we don't care about the limitations of drive hardware
		However, VICE (previous to version 2.2) ignored the G64 header and hardcodes 7928 as the largest
		track size, and also requires it to be 84 tracks no matter if they're used or not.
	*/

	#define OLD_G64_TRACK_MAXLEN 8192
	DWORD G64_TRACK_MAXLEN=7928;
	BYTE header[12];
	DWORD gcr_track_p[MAX_HALFTRACKS_1541] = {0};
	DWORD gcr_speed_p[MAX_HALFTRACKS_1541] = {0};
	//BYTE gcr_track[G64_TRACK_MAXLEN + 2];
	BYTE gcr_track[NIB_TRACK_LENGTH + 2];
	size_t track_len, badgcr;
	//size_t skewbytes=0;
	int index=0, track, added_sync;
	FILE * fpout;
	BYTE buffer[NIB_TRACK_LENGTH], tempfillbyte;
	size_t raw_track_size[4] = { 6250, 6666, 7142, 7692 };
	//char errorstring[0x1000];

	printf("Writing G64 file...\n");

	fpout = fopen(filename, "wb");
	if (fpout == NULL)
	{
		printf("Cannot open G64 image %s.\n", filename);
		return 0;
	}

	/* determine max track size (VICE still can't handle) */
	//for (index= 0; index < MAX_HALFTRACKS_1541; index += track_inc)
	//{
	//	if(track_length[index+2] > G64_TRACK_MAXLEN)
	//		G64_TRACK_MAXLEN = track_length[index+2];
	//}
	printf("G64 Track Length = %d", G64_TRACK_MAXLEN);

	/* Create G64 header */
	strcpy((char *) header, "GCR-1541");
	header[8] = 0;	/* G64 version */
	header[9] = MAX_HALFTRACKS_1541; /* Number of Halftracks  (VICE <2.2 can't handle non-84 track images) */
	//header[9] = (unsigned char)end_track;
	header[10] = (BYTE) (G64_TRACK_MAXLEN % 256);	/* Size of each stored track */
	header[11] = (BYTE) (G64_TRACK_MAXLEN / 256);

	if (fwrite(header, sizeof(header), 1, fpout) != 1)
	{
		printf("Cannot write G64 header.\n");
		return 0;
	}

	/* Create track and speed tables */
	for (track = 0; track < MAX_HALFTRACKS_1541; track +=track_inc)
	{
		/* calculate track positions and speed zone data */
		if((!old_g64)&&(!track_length[track+2])) continue;

		gcr_track_p[track] = 0xc + (MAX_TRACKS_1541 * 16) + (index++ * (G64_TRACK_MAXLEN + 2));
		gcr_speed_p[track] = track_density[track+2]&3;
	}

	/* write headers */
	if (write_dword(fpout, gcr_track_p, sizeof(gcr_track_p)) < 0)
	{
		printf("Cannot write track header.\n");
		return 0;
	}

	if (write_dword(fpout, gcr_speed_p, sizeof(gcr_speed_p)) < 0)
	{
		printf("Cannot write speed header.\n");
		return 0;
	}

	/* shuffle raw GCR between formats */
	for (track = 2; track <= MAX_HALFTRACKS_1541+1; track +=track_inc)
	{
		track_len = track_length[track];
		if(track_len>G64_TRACK_MAXLEN) track_len=G64_TRACK_MAXLEN;

		if((!old_g64)&&(!track_len)) continue;

		/* loop last byte of track data for filler */
		if(fillbyte == 0xfe) /* $fe is special case for loop */
			tempfillbyte = track_buffer[(track * NIB_TRACK_LENGTH) + track_len - 1];
		else
			tempfillbyte = fillbyte;

		memset(&gcr_track[2], tempfillbyte, G64_TRACK_MAXLEN);

		gcr_track[0] = (BYTE) (raw_track_size[speed_map[track/2]] % 256);
		gcr_track[1] = (BYTE) (raw_track_size[speed_map[track/2]] / 256);

		memcpy(buffer, track_buffer + (track * NIB_TRACK_LENGTH), track_len);

		/* process/compress GCR data */
		badgcr = check_bad_gcr(buffer, track_len);

		if(increase_sync)
		{
			added_sync = lengthen_sync(track_buffer + (NIB_TRACK_LENGTH * track),
			track_len, NIB_TRACK_LENGTH);

			if(verbose) printf(" [sync:%d] ", added_sync);
			track_len += added_sync;
		}

		if(rpm_real)
		{
			//capacity[speed_map[track/2]] = raw_track_size[speed_map[track/2]];
			switch (track_density[track])
			{
				case 0:
					capacity[speed_map[track/2]] = (size_t)(DENSITY0/rpm_real);
					break;
				case 1:
					capacity[speed_map[track/2]] = (size_t)(DENSITY1/rpm_real);
					break;
				case 2:
					capacity[speed_map[track/2]] = (size_t)(DENSITY2/rpm_real);
					break;
				case 3:
					capacity[speed_map[track/2]] = (size_t)(DENSITY3/rpm_real);
				break;
			}

			if(capacity[speed_map[track/2]] > G64_TRACK_MAXLEN)
				capacity[speed_map[track/2]] = G64_TRACK_MAXLEN;

			/* user display */
			if(verbose)
			{
				printf("\n%4.1f: (", (float)track/2);
				printf("%d", track_density[track]&3);
				if ( (track_density[track]&3) != speed_map[track/2]) printf("!");
				printf(":%d) ", track_length[track]);
				if (track_density[track] & BM_NO_SYNC) printf("NOSYNC ");
				if (track_density[track] & BM_FF_TRACK) printf("KILLER ");
			}

			track_len = compress_halftrack(track, buffer, track_density[track], track_len);
			if(verbose) printf("(%d)", track_len);
		}
		else
		{
			capacity[speed_map[track/2]] = G64_TRACK_MAXLEN;
			track_len = compress_halftrack(track, buffer, track_density[track], track_len);
		}
		if(verbose>1) printf("(fill:$%.2x) ",tempfillbyte);
		if(verbose>1) printf("{badgcr:%d}",badgcr);

		gcr_track[0] = (BYTE) (track_len % 256);
		gcr_track[1] = (BYTE) (track_len / 256);

		/* apply skew, if specified */
		//if(skew)
		//{
		//	skewbytes = skew * (capacity[track_density[track]&3] / 200);
		//	if(skewbytes > track_len)
		//		skewbytes = skewbytes - track_len;
		//printf(" {skew=%d} ", skewbytes);
		//}
		//memcpy(gcr_track+2, buffer+skewbytes, track_len-skewbytes);
		//memcpy(gcr_track+2+track_len-skewbytes, buffer, skewbytes);

		memcpy(gcr_track+2, buffer, track_len);

		if (fwrite(gcr_track, (G64_TRACK_MAXLEN + 2), 1, fpout) != 1)
		{
			printf("Cannot write track data.\n");
			return 0;
		}
	}
	fclose(fpout);
	printf("\nSuccessfully saved G64 file\n");
	return 1;
}

size_t compress_halftrack(int halftrack, BYTE *track_buffer, BYTE density, size_t length)
{
	size_t orglen;
	BYTE gcrdata[NIB_TRACK_LENGTH];

	/* copy to spare buffer */
	memcpy(gcrdata, track_buffer, NIB_TRACK_LENGTH);
	memset(track_buffer, 0, NIB_TRACK_LENGTH);

	/* process and compress track data (if needed) */
	if (length > 0)
	{
		/* If our track contains sync, we reduce to a minimum of 32 bits
		   less is too short for some loaders including CBM, but only 10 bits are technically required */
		orglen = length;
		if ( (length > (capacity[density&3])) && (!(density & BM_NO_SYNC)) &&
			(reduce_map[halftrack/2] & REDUCE_SYNC) )
		{
			/* reduce sync marks within the track */
			length = reduce_runs(gcrdata, length, capacity[density&3], reduce_sync, 0xff);
			if(verbose>1) printf("(-sync:%d)", orglen - length);
		}

		/* reduce bad GCR runs */
		orglen = length;
		if ( (length > (capacity[density&3])) &&
			(reduce_map[halftrack/2] & REDUCE_BAD) )
		{
			length = reduce_runs(gcrdata, length, capacity[density&3], 0, 0x00);
			if(verbose) printf("(-badgcr:%d)", orglen - length);
		}

		/* reduce sector gaps -  they occur at the end of every sector and vary from 4-19 bytes, typically  */
		orglen = length;
		if ( (length > (capacity[density&3])) &&
			(reduce_map[halftrack/2] & REDUCE_GAP) )
		{
			length = reduce_gaps(gcrdata, length, capacity[density & 3]);
			if(verbose) printf("(-gap:%d)", orglen - length);
		}

		/* still not small enough, we have to truncate the end (reduce tail) */
		orglen = length;
		if (length > capacity[density&3])
		{
			length = capacity[density&3];
			if(verbose>1) printf("(-trunc:%d)", orglen - length);
		}
	}

	/* if track is empty (unformatted) fill with '0' bytes to simulate */
	if ( (!length) && (density & BM_NO_SYNC))
	{
		memset(gcrdata, 0, NIB_TRACK_LENGTH);
		length = NIB_TRACK_LENGTH;
	}

	/* write processed track buffer */
	memcpy(track_buffer, gcrdata, length);
	return length;
}

BYTE compressed_buffer[(MAX_HALFTRACKS_1541 + 2) * NIB_TRACK_LENGTH];
BYTE file_buffer[(MAX_HALFTRACKS_1541 + 2) * NIB_TRACK_LENGTH];
BYTE track_buffer[(MAX_HALFTRACKS_1541 + 2) * NIB_TRACK_LENGTH];
BYTE track_density[MAX_HALFTRACKS_1541 + 2];
BYTE track_alignment[MAX_HALFTRACKS_1541 + 2];
size_t track_length[MAX_HALFTRACKS_1541 + 2];
int file_buffer_size;
int start_track, end_track, track_inc;
int reduce_sync, reduce_badgcr, reduce_gap;
int fix_gcr, align, force_align;
int gap_match_length;
int cap_min_ignore;
int skip_halftracks;
int verbose;
int rpm_real;
int auto_capacity_adjust;
int skew;
int align_disk;
int ihs;
int mode;
int unformat_passes;
int capacity_margin;
int align_delay;
int increase_sync = 0;
int presync = 0;
BYTE fillbyte = 0x55;
BYTE drive = 8;
char * cbm_adapter = "";
int use_floppycode_srq = 0;
int extra_capacity_margin=5;
int sync_align_buffer=0;
int fattrack=0;
int track_match=0;
int old_g64=0;

int nib_convert(char *in, char *out)
{
	char inname[256], outname[256];
	//char *dotpos;
	//FILE *fp;
	int t;

	start_track = 1 * 2;
	end_track = 42 * 2;
	track_inc = 1;
	fix_gcr = 1;
	reduce_sync = 4;
	skip_halftracks = 0;
	align = ALIGN_NONE;
	force_align = ALIGN_NONE;
	gap_match_length = 7;
	cap_min_ignore = 0;
	verbose = 0;
	rpm_real = 296;

	/* default is to reduce sync */
	//memset(reduce_map, REDUCE_SYNC, MAX_TRACKS_1541 + 2); 
	/* Lowered + 2 to + 1, because: "warning: 'memset' writing 44 bytes into a region of size 43 overflows the destination" */
	memset(reduce_map, REDUCE_SYNC, MAX_TRACKS_1541 + 1);

	//memset(track_length, 0, MAX_TRACKS_1541 + 2);
	for(t=0; t<MAX_TRACKS_1541 + 2; t++)
		track_length[t] = NIB_TRACK_LENGTH; // I do not recall why this was done, but left at MAX

	/* clear heap buffers */
	memset(compressed_buffer, 0x00, sizeof(compressed_buffer));
	memset(file_buffer, 0x00, sizeof(file_buffer));
	memset(track_buffer, 0x00, sizeof(track_buffer));

	snprintf(inname, sizeof(inname), "%s", in);//strcpy(inname, in);
	snprintf(outname, sizeof(outname), "%s", out);//strcpy(outname, out);

	/* convert */
	if (compare_extension((unsigned char *)inname, (unsigned char *)"NIB"))
	{
		if(!(file_buffer_size = load_file(inname, file_buffer))) return 0;
		if(!(read_nib(file_buffer, file_buffer_size, track_buffer, track_density, track_length))) return 0;
		if( (compare_extension((unsigned char *)outname, (unsigned char *)"G64")) || (compare_extension((unsigned char *)outname, (unsigned char *)"D64")) )
			align_tracks(track_buffer, track_density, track_length, track_alignment);
		search_fat_tracks(track_buffer, track_density, track_length);
	}

	if (compare_extension((unsigned char *)outname, (unsigned char *)"G64"))
	{
		if(skip_halftracks) track_inc = 2;
		if(!(write_g64(outname, track_buffer, track_density, track_length))) return 0;
	}

	return 1;
}
