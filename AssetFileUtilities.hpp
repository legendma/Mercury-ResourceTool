#pragma once
#include <stdio.h>

#include "Global.hpp"

static __inline b8 file_read_buffer( fhnd hnd, const u64 buffer_sz, const u64 read_sz, void *buffer );


/*******************************************************************
*
*   file_close()
*
*******************************************************************/

static __inline b8 file_close( fhnd hnd )
{
int result = fclose( (FILE*)hnd );
return( result == 0 );

}   /* file_close() */


/*******************************************************************
*
*   file_delete()
*
*******************************************************************/

static __inline b8 file_delete( const char *filename_w_path )
{
return( !remove( filename_w_path ) );

}   /* file_delete() */


/*******************************************************************
*
*   file_get_pos()
*
*******************************************************************/

static __inline u64 file_get_pos( fhnd hnd )
{
long result = ftell( (FILE*)hnd );
if( result < 0 )
    {
    return( 0 );
    }

return( (u64)result );

}   /* file_get_pos() */


/*******************************************************************
*
*   file_open()
*
*******************************************************************/

static __inline b8 file_open( const char *filename_w_path, const char *mode, fhnd *hnd )
{
*hnd = (fhnd)fopen( filename_w_path, mode );

return( *hnd != 0 );

}   /* file_open() */


/*******************************************************************
*
*   file_read()
*
*******************************************************************/

static __inline b8 file_read( fhnd hnd, const u64 read_sz, void *out )
{
return( file_read_buffer( hnd, read_sz, read_sz, out ) );

}   /* file_read() */


/*******************************************************************
*
*   file_read_array()
*
*******************************************************************/

#define file_read_array( _fhnd, _cnt, _ptype ) \
    file_read( _fhnd, (_cnt) * sizeof( *(_ptype) ), _ptype )


/*******************************************************************
*
*   file_read_struct()
*
*******************************************************************/

#define file_read_struct( _fhnd, _ptype ) \
    file_read_array( _fhnd, 1, _ptype )


/*******************************************************************
*
*   file_read_buffer()
*
*******************************************************************/

static __inline b8 file_read_buffer( fhnd hnd, const u64 buffer_sz, const u64 read_sz, void *buffer )
{
if( read_sz > buffer_sz )
    {
    return( FALSE );
    }

u64 got_sz = (u64)fread( buffer, 1, read_sz, (FILE*)hnd );
return( got_sz == read_sz );

}   /* file_read_buffer() */


/*******************************************************************
*
*   file_rename()
*
*******************************************************************/

static __inline b8 file_rename( const char *from_filename_w_path, const char *to_filename_w_path )
{
return( !rename( from_filename_w_path, to_filename_w_path ) );

}   /* file_rename() */


/*******************************************************************
*
*   file_rewind()
*
*******************************************************************/

static __inline void file_rewind( fhnd hnd )
{
rewind( (FILE*)hnd );

}   /* file_rewind() */


/*******************************************************************
*
*   file_seek()
*
*******************************************************************/

static __inline b8 file_seek( fhnd hnd, const u64 location )
{
int result = fseek( (FILE*)hnd, (long)location, SEEK_SET );
return( result == 0 );

}   /* file_seek() */


/*******************************************************************
*
*   file_seek_rel()
*
*******************************************************************/

static __inline b8 file_seek_rel( fhnd hnd, const s64 offset )
{
int result = fseek( (FILE*)hnd, (long)offset, SEEK_CUR );
return( result == 0 );

}   /* file_seek_rel() */


/*******************************************************************
*
*   file_write()
*
*******************************************************************/

static __inline b8 file_write( fhnd hnd, const u64 sz, const void *data )
{
u64 did_sz = (u64)fwrite( data, 1, sz, (FILE*)hnd );
return( did_sz == sz );

}   /* file_write() */


/*******************************************************************
*
*   file_write_array()
*
*******************************************************************/

#define file_write_array( _fhnd, _cnt, _ptype ) \
    file_write( _fhnd, (_cnt) * sizeof( *(_ptype) ), _ptype )


/*******************************************************************
*
*   file_write_struct()
*
*******************************************************************/

#define file_write_struct( _fhnd, _ptype ) \
    file_write_array( _fhnd, 1, _ptype )


