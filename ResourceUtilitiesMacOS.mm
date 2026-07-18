#include <string>

#import <Foundation/Foundation.h>


/*******************************************************************
*
*   create_dir()
*
*   DESCRIPTION:
*       Create the requested directory at the current directory.
*
*******************************************************************/

void create_dir( const char *name )
{
NSFileManager *manager = [ NSFileManager defaultManager ];
NSString *ns_pathname = [ NSString stringWithUTF8String:name ];
[ manager createDirectoryAtPath:ns_pathname
                                withIntermediateDirectories:true
                                attributes:nil
                                error:nil ];

} /* create_dir() */


/*******************************************************************
*
*   does_file_exist()
*
*   DESCRIPTION:
*       Does the given file exist?
*
*******************************************************************/

bool does_file_exist( const char *filename )
{
NSFileManager *manager = [ NSFileManager defaultManager ];
NSString *ns_filename = [ NSString stringWithUTF8String:filename ];
return( [ manager fileExistsAtPath:ns_filename ] != false );

} /* does_file_exist() */


/*******************************************************************
*
*   get_current_dir_str()
*
*   DESCRIPTION:
*       Query the current working directory as a string.
*
*******************************************************************/

std::string get_current_dir_str()
{
NSFileManager *manager = [ NSFileManager defaultManager ];
NSString *cwd = [ manager currentDirectoryPath ];

std::string ret = std::string( [ cwd UTF8String] );
return( ret );

} /* get_current_dir_str() */