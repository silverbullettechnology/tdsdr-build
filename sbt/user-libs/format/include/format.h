/** \file      format.h
 *  \brief     I/O formatters - public definitions
 *  \copyright Copyright 2013,2014 Silver Bullet Technology
 *
 *             Licensed under the Apache License, Version 2.0 (the "License"); you may not
 *             use this file except in compliance with the License.  You may obtain a copy
 *             of the License at: 
 *               http://www.apache.org/licenses/LICENSE-2.0
 *
 *             Unless required by applicable law or agreed to in writing, software
 *             distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *             WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 *             License for the specific language governing permissions and limitations
 *             under the License.
 *
 * vim:ts=4:noexpandtab
 */
#ifndef _INCLUDE_FORMAT_H_
#define _INCLUDE_FORMAT_H_
#include <stdio.h>


/* Callback for progress reporting to the user, see below for details */
typedef void (* format_progress_fn) (size_t done, size_t size);


/* Bits for flags bitmap */
#define  FO_ENDIAN   0x00000001  /* Swap sample endian on load/save    */
#define  FO_IQ_SWAP  0x00000002  /* Swap order of I and Q on load/save */


/* Options struct describes an application's requirements for how samples are arranged in
 * the data buffer.  If an options struct is not passed to one of the functions below, a
 * default struct is used which describes a 2-channel buffer of 16-bit subsamples, I,Q
 * order, with no space reserved for packetization. */
struct format_options
{
	/* "channels" gives the channels to operate on within a buffer, it's expected the user
	 * will set/clear bits in this before an operation.  For example, when loading a
	 * 2-channel buffer from a single-channel format, the user could set "channels" to 
	 * (1 << 0) | (1 << 1) to operate on both channels in a single pass. */
	unsigned long  channels;

	/* "single" size is the size of a single complex sample (I and Q parts), default is 
	 * (sizeof(uint16_t) * 2) or 4 bytes.  */
	size_t  single;  

	/* "sample" size gives the size of a group of simultaneous samples; for single-channel
	 * buffers, it will equal the "single" value.  For a 2-channel buffer it will be
	 * double the "single" value. */
	size_t  sample;

	/* "i_bits" and "q_bits) describes the number of significant bits in each subsample,
	 * in case it's less than implied by "single" */
	size_t  bits;

	/* "packet" is used for buffers which contain sample data in a packet format.
	 * "packet" gives the size of each packet in bytes, "head" gives the size reserved for
	 * the header on each packet, "data" for the data bytes, and "foot" for the footer on
	 * each packet.  If "packet" is zero then the sample data is assumed packed into the
	 * buffer without gaps */
	size_t  packet;
	size_t  head;
	size_t  data;
	size_t  foot;

	/* If not NULL, "prog_func" will be called periodically to update the user on the
	 * progress of a load/save operation.  the function is called in one of four ways
	 * depending on the situation:
	 * - called with size = the buffer size and done = 0 before starting
	 * - called with size = the buffer size and done < size during the operation
	 * - called with size = the buffer size and done = size after completion
	 * - called with size = 0 and done = 0 before an error return
	 *
	 * If "prog_step" is nonzero then the progress calls will come each time at least
	 * "prog_step" bytes of data have been processed, including packet overhead.  This is
	 * "at least" because the "prog_step" size and actual block size processed may differ.
	 * If "prog_step" is zero then the function is called after each internal block.
	 */
	format_progress_fn  prog_func;
	size_t              prog_step;

	/* Bitmap of FO_* flags above */
	unsigned long  flags;

	/* Normally the number of data bytes read/written is calculated from the buffer size
	 * passed, adjusted for packet header/footer space.  If "limit" is nonzero and less
	 * than the buffer size, read/write operations will stop after this many data bytes
	 * have been processed, allowing for padding/truncation to fit fabric restrictions */
	size_t  limit;
};


/** Opaque struct for format class */
struct format_class;


/** Find a class by name
 *
 *  \param  name  Class name specified by user
 *
 *  \return  Class struct pointer, or NULL on error
 */
struct format_class *format_class_find (const char *name);

/** Guess a class by filename
 *
 *  Currently uses the file extension (after the last dot), may in future also search a
 *  list of possible extensions.
 *
 *  \param  name  Filename specified by user
 *
 *  \return  Class struct pointer, or NULL on error
 */
struct format_class *format_class_guess (const char *name);

/** Return a class's printable name
 *
 *  \param  fmt  Format class pointer
 *
 *  \return  Printable string, which may be "(null)" if the class pointer is NULL
 */
const char *format_class_name (struct format_class *fmt);

/** Print a list of compiled-in classes to the given file handle
 *
 *  The list will contain at minimum each class name and in/out/both support, and a
 *  description for classes which implement it.
 *
 *  \param  fp  Output FILE pointer
 */
void format_class_list (FILE *fp);

/** Returns a hint as to how many channels a single call of _read or _write can handle,
 *  for a given class
 *
 *  - FC_CHAN_BUFFER - operates on the entire buffer and all channels in it (ie, .bin)
 *  - FC_CHAN_SINGLE - operates on a single channel (not currently used)
 *  - FC_CHAN_MULTI  - operates on multiple channels, specified by the "channels" bitmap
 *                     in the options struct.
 *
 *  Note that some FC_CHAN_MULTI formats (ie IQW) handle inherently single-channel data
 *  files but can be loaded into multiple channels in a single pass for efficiency.  These
 *  formats may return different values for calls to format_class_read_channels() versus
 *  format_class_write_channels().
 *
 *  Note also that FC_CHAN_BUFFER formats generally work on the packed binary format in
 *  the buffer, and calling one with a channels mask which isn't the full width of the
 *  buffer will trigger a warning if debug is enabled.
 *
 *  \param  fmt  Format class pointer
 *
 *  \return  One of the FC_CHAN_* constants described above, or <0 if the class pointer is
 *           NULL (errno will be EFAULT) or the class doesn't support the relevant
 *           operation (ie _read on an output-only format, errno will be ENOSYS)
 */
#define FC_CHAN_BUFFER 0  
#define FC_CHAN_SINGLE 1  
#define FC_CHAN_MULTI  2  
int format_class_read_channels  (struct format_class *fmt);
int format_class_write_channels (struct format_class *fmt);


/** Analyze an existing file and calculate buffer size
 *
 *  Reads the file as needed and calculates the number of data samples contained, and the
 *  number of buffer bytes necessary to load it according to the options given.  Note this
 *  may be larger than the file-size: if options specifies a 2-channel buffer, sizing a
 *  single-channel format will return the size of a 2-channel buffer size.
 *
 *  If opt is NULL, an internal defaults struct is used instead, which matches the SDRDC
 *  legacy: two channels of 12-bit I/Q with 16-bit alignment, packed with no gaps for
 *  packet headers.
 *
 *  \param  fmt  Format class pointer
 *  \param  opt  Format options pointer
 *  \param  fp   FILE pointer
 *
 *  \return  Buffer size in bytes, or 0 on error
 */
size_t format_size (struct format_class *fmt, struct format_options *opt, FILE *fp);


/** Read a file into a buffer
 *
 *  If the buffer is smaller than the file the data will be truncated to fit.  If the
 *  buffer is larger then the fill will be rewound and looped to fit.  Currently this
 *  means fp cannot be a stream, this restriction may be lifted in future.
 *
 *  The "packet" fields in the options if present will cause the operation to skip bytes
 *  in the buffer, so the user can pre-fill the packet headers/footers if desired. 
 *
 *  Multi-channel details, based on the return from format_class_read_channels():
 *  - FC_CHAN_BUFFER: all channels in the buffer will be loaded regardless of the
 *                    "channels" field in the options.   
 *  - FC_CHAN_MULTI:  the channel with the lowest-numbered bit set in "channels" will be
 *                    loaded with the first channel in each read sample, the next-lowest
 *                    the second channel, etc.
 *  - FC_CHAN_SINGLE: each channel with a bit set in "channels" will be loaded with the
 *                    read data. 
 *
 *  \param  fmt   Format class pointer
 *  \param  opt   Format options pointer
 *  \param  buff  Buffer to read into
 *  \param  size  Buffer size
 *  \param  fp    FILE pointer
 *
 *  \return  number of bytes used in the buffer, which should be size, or 0 on error.
 */
size_t format_read (struct format_class *fmt, struct format_options *opt,
                    void *buff, size_t size, FILE *fp);


/** Write a file from a buffer
 *
 *  The "packet" fields in the options if present will cause the operation to skip bytes
 *  in the buffer, so the user can pre-fill the packet headers/footers if desired. 
 *
 *  Multi-channel details, based on the return from format_class_write_channels():
 *  - FC_CHAN_BUFFER: all channels in the buffer will be written regardless of the
 *                    "channels" field in the options.   
 *  - FC_CHAN_MULTI:  the channel with the lowest-numbered bit set in "channels" will be
 *                    the first channel written in each output sample, the next-lowest the
 *                    second channel, etc.
 *  - FC_CHAN_SINGLE: the channel with the only bit set in "channels" will be written; a
 *                    single bit must be set before writing - otherwise is an error.
 *
 *  \param  fmt   Format class pointer
 *  \param  opt   Format options pointer
 *  \param  buff  Buffer to write from
 *  \param  size  Buffer size
 *  \param  fp    FILE pointer
 *
 *  \return  number of bytes used in the buffer, which should be size, or 0 on error.
 */
size_t format_write (struct format_class *fmt, struct format_options *opt,
                     const void *buff, size_t size, FILE *fp);


/** Calculate payload data size from a given buffer size
 *
 *  This mostly calculates and subtracts the packet overhead according to the given
 *  options.
 *
 *  \param  opt   Format options pointer
 *  \param  buff  Size of buffer in bytes
 *
 *  \return  Size of the output data in the buffer, or 0 on error
 */
size_t format_size_data_from_buff (struct format_options *opt, size_t buff);

/** Calculate buffer size from a given payload data size
 *
 *  This mostly calculates and adds the packet overhead according to the given options. 
 *
 *  \param  opt   Format options pointer
 *  \param  data  Size of payload data in bytes
 *
 *  \return  Size of the buffer, or 0 on error
 */
size_t format_size_buff_from_data (struct format_options *opt, size_t data);


/** Calculate number of packets from a given buffer size
 *
 *  \param  opt   Format options pointer
 *  \param  buff  Size of buffer in bytes
 *
 *  \return  Number of packets in the buffer, or 0 on error
 */
size_t format_num_packets_from_buff (struct format_options *opt, size_t buff);

/** Calculate number of packets from a given payload data size
 *
 *  \param  opt   Format options pointer
 *  \param  data  Size of payload data in bytes
 *
 *  \return  Number of packets in the buffer, or 0 on error
 */
size_t format_num_packets_from_data (struct format_options *opt, size_t data);


/** Setup the output FILE handle for error messages
 * 
 *  \note  if set to NULL the library will try to use stderr to print an error message.
 *         To discard all error messages, use something like:
 *           format_error_setup(fopen("/dev/null", "w"));
 *
 *  \param  fp  FILE pointer
 */
void format_error_setup (FILE *fp);

/** Setup the output FILE handle for debug messages
 *
 *  \note  if set to NULL no debug messaeges will be printed.
 *
 *  \param  fp  FILE pointer
 */
void format_debug_setup (FILE *fp);


#endif /* _INCLUDE_FORMAT_H_ */
