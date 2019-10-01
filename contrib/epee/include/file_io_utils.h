// Copyright (c) 2006-2013, Andrey N. Sabelnikov, www.sabelnikov.net
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// * Neither the name of the Andrey N. Sabelnikov nor the
// names of its contributors may be used to endorse or promote products
// derived from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER  BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 


#ifndef _FILE_IO_UTILS_H_
#define _FILE_IO_UTILS_H_


//#include <sys/types.h>
//#include <sys/stat.h>

#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#ifndef MAKE64
	#define MAKE64(low,high)	((__int64)(((DWORD)(low)) | ((__int64)((DWORD)(high))) << 32))
#endif

#ifdef WINDOWS_PLATFORM
#include <psapi.h>
#include <strsafe.h>
#include <string.h>
#include <mbstring.h>
#endif

#ifndef  WIN32
#include <sys/file.h>
#endif

#include "include_base_utils.h"
#include "string_coding.h"

namespace epee
{
namespace file_io_utils
{
#ifdef WINDOWS_PLATFORM

	inline 
	std::string get_temp_file_name_a()
	{
		std::string str_result;
		char	sz_temp[MAX_PATH*2] = {0};
		if(!::GetTempPathA( sizeof( sz_temp ), sz_temp ))
			return str_result;
		
		char	sz_temp_file[MAX_PATH*2] = {0};
		if(!::GetTempFileNameA( sz_temp, "mail", 0, sz_temp_file))
			return str_result;
		sz_temp_file[sizeof(sz_temp_file)-1] = 0; //be happy!
		str_result = sz_temp_file;
		return str_result;
	}


#ifdef BOOST_LEXICAL_CAST_INCLUDED
	inline
	bool get_not_used_filename(const std::string& folder, OUT std::string& result_name)
	{	
		DWORD folder_attr = ::GetFileAttributesA(folder.c_str());
		if(folder_attr == INVALID_FILE_ATTRIBUTES)
			return false;
		if(!(folder_attr&FILE_ATTRIBUTE_DIRECTORY))
			return false;


		std::string base_name = folder + "\\tmp";
		std::string tmp_name;
		bool name_found = false;
		int current_index = 0;
		tmp_name = base_name + boost::lexical_cast<std::string>(current_index) + ".tmp";
		while(!name_found)
		{
			if(INVALID_FILE_ATTRIBUTES == ::GetFileAttributesA(tmp_name.c_str()))
				name_found = true;
			else
			{
				current_index++;
				tmp_name = base_name + boost::lexical_cast<std::string>(current_index) + ".tmp";
			}
		}
		result_name = tmp_name;
		return true;
	}
#endif
	
	inline 
		std::string get_temp_folder_a()
	{
		std::string str_result;
		char	sz_temp[MAX_PATH*2] = {0};
		if(!::GetTempPathA( sizeof( sz_temp ), sz_temp ))
			return str_result;
		sz_temp[(sizeof(sz_temp)/sizeof(sz_temp[0])) -1] = 0;
		str_result = sz_temp;
		return str_result;
	}

	std::string convert_from_device_path_to_standart(const std::string& path)
	{


		STRSAFE_LPSTR pszFilename = (STRSAFE_LPSTR)path.c_str();

		// Translate path with device name to drive letters.
		char szTemp[4000] = {0};

		if (::GetLogicalDriveStringsA(sizeof(szTemp)-1, szTemp)) 
		{
			char szName[MAX_PATH];
			char szDrive[3] = " :";
			BOOL bFound = FALSE;
			char* p = szTemp;

			do 
			{
				// Copy the drive letter to the template string
				*szDrive = *p;

				// Look up each device name
				if (::QueryDosDeviceA(szDrive, szName, sizeof(szName)))
				{
					UINT uNameLen = strlen(szName);

					if (uNameLen < MAX_PATH) 
					{
						bFound = _mbsnbicmp((const unsigned char*)pszFilename, (const unsigned char*)szName, 
							uNameLen) == 0;

						if (bFound) 
						{
							// Reconstruct pszFilename using szTempFile
							// Replace device path with DOS path
							char szTempFile[MAX_PATH] = {0};
							StringCchPrintfA(szTempFile,
								MAX_PATH,
								"%s%s",
								szDrive,
								pszFilename+uNameLen);
							return szTempFile;
							//::StringCchCopyNA(pszFilename, MAX_PATH+1, szTempFile, strlen(szTempFile));
						}
					}
				}

				// Go to the next NULL character.
				while (*p++);
			} while (!bFound && *p); // end of string
		}

		return "";
	}
	
	inline 
		std::string get_process_path_by_pid(DWORD pid)
	{
		std::string res;

		HANDLE hprocess = 0;
		if( hprocess = ::OpenProcess( PROCESS_QUERY_INFORMATION|PROCESS_VM_READ, FALSE, pid) )
		{
			char buff[MAX_PATH]= {0};
			if(!::GetModuleFileNameExA( hprocess, 0, buff, MAX_PATH - 1 ))
				res = "Unknown_b";
			else 
			{
				buff[MAX_PATH - 1]=0; //be happy!
				res = buff;
				std::string::size_type a = res.rfind( '\\' );
				if ( a != std::string::npos )
					res.erase( 0, a+1);					

			}
			::CloseHandle( hprocess );
		}else
			res = "Unknown_a";

		return res;
	}





	inline 
	std::wstring get_temp_file_name_w()
	{
		std::wstring str_result;
		wchar_t	sz_temp[MAX_PATH*2] = {0};
		if(!::GetTempPathW( sizeof(sz_temp)/sizeof(sz_temp[0]), sz_temp ))
			return str_result;

		wchar_t	sz_temp_file[MAX_PATH+1] = {0};
		if(!::GetTempFileNameW( sz_temp, L"mail", 0, sz_temp_file))
			return str_result;

		sz_temp_file[(sizeof(sz_temp_file)/sizeof(sz_temp_file[0]))-1] = 0; //be happy!
		str_result = sz_temp_file;
		return str_result;
	}
#endif

  inline const std::wstring& convert_utf8_to_wstring_if_needed(const std::wstring& s)
  {
    return s;
  }

  inline std::wstring convert_utf8_to_wstring_if_needed(const std::string& s)
  {
    return epee::string_encoding::utf8_to_wstring(s);
  }
	 
  template<class t_string>
  bool is_file_exist(const t_string& path)
  {
    boost::filesystem::path p(convert_utf8_to_wstring_if_needed(path));
    return boost::filesystem::exists(p);
  }

	/*
	inline 
		bool save_string_to_handle(HANDLE hfile, const std::string& str)
	{
		


		if( INVALID_HANDLE_VALUE != hfile )
		{
			DWORD dw;
			if( !::WriteFile( hfile, str.data(), (DWORD) str.size(), &dw, NULL) )
			{
				int err_code = GetLastError();
				//LOG_PRINT("Failed to write to file handle: " << hfile<< " Last error code:" << err_code << " : " << log_space::get_win32_err_descr(err_code), LOG_LEVEL_2);
				return false;
			}
			::CloseHandle(hfile);
			return true;
		}else
		{
			//LOG_WIN32_ERROR(::GetLastError());
			return false;
		}

		return false;
	}*/


  template<class t_string>
  bool save_string_to_file_throw(const t_string& path_to_file, const std::string& str)
  {
    //std::ofstream fstream;
    boost::filesystem::ofstream fstream;
    fstream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fstream.open(convert_utf8_to_wstring_if_needed(path_to_file), std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
    fstream << str;
    fstream.close();
    return true;
  }

	template<class t_string>
  bool save_string_to_file(const t_string& path_to_file, const std::string& str)
	{
		try
		{
      return save_string_to_file_throw(path_to_file, str);
		}
    catch (const std::exception& /*ex*/)
    {
      return false;
    }

		catch(...)
		{
			return false;
		}
	}

  template<class t_string>
  bool load_file_to_string(const t_string& path_to_file, std::string& target_str)
  {
    try
    {
      boost::filesystem::ifstream  fstream;
      //fstream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
      fstream.open(convert_utf8_to_wstring_if_needed(path_to_file), std::ios_base::binary | std::ios_base::in | std::ios::ate);
      if (!fstream.good())
        return false;
      std::ifstream::pos_type file_size = fstream.tellg();

      if (file_size > 1000000000)
        return false;//don't get crazy
      size_t file_size_t = static_cast<size_t>(file_size);

      target_str.resize(file_size_t);

      fstream.seekg(0, std::ios::beg);
      fstream.read((char*)target_str.data(), target_str.size());
      if (!fstream.good())
        return false;

      fstream.close();
      return true;
    }
    catch (...)
    {
      return false;
    }
  }


	/*
	inline
		bool load_form_handle(HANDLE hfile, std::string& str)
	{
		if( INVALID_HANDLE_VALUE != hfile )
		{
			bool res = true;
			DWORD dw = 0;
			DWORD fsize = ::GetFileSize(hfile, &dw);
			if(fsize > 300000000)
			{
				::CloseHandle(hfile);
				return false;
			}
			if(fsize)
			{
				str.resize(fsize);
				if(!::ReadFile( hfile, (LPVOID)str.data(), (DWORD)str.size(), &dw, NULL))
					res = false;
			}
			::CloseHandle(hfile);
			return res;
		}
		return false;
	}
	*/
	inline
	bool get_file_time(const std::string& path_to_file, OUT time_t& ft)
	{
		boost::system::error_code ec;
		ft = boost::filesystem::last_write_time(epee::string_encoding::utf8_to_wstring(path_to_file), ec);
		if(!ec)
			return true;
		else
			return false;
	}

	inline
		bool set_file_time(const std::string& path_to_file, const time_t& ft)
	{
		boost::system::error_code ec;
		boost::filesystem::last_write_time(epee::string_encoding::utf8_to_wstring(path_to_file), ft, ec);
		if(!ec)
			return true;
		else
			return false;
	}




#ifdef WIN32
  typedef HANDLE native_filesystem_handle;
#else
  typedef int native_filesystem_handle;
#endif

  // uses UTF-8 for unicode names for all systems
  inline bool open_and_lock_file(const std::string file_path, native_filesystem_handle& h_file)
  {
#ifdef WIN32
    std::wstring file_path_w = epee::string_encoding::utf8_to_wstring(file_path);
    h_file = ::CreateFileW(file_path_w.c_str(), // name of the file
      GENERIC_WRITE,                            // open for writing
      0,                                        // do not share
      NULL,                                     // default security
      OPEN_ALWAYS,                              // create new file only
      FILE_ATTRIBUTE_NORMAL,                    // normal file
      NULL);                                    // no attr. template
    if (h_file == INVALID_HANDLE_VALUE)
      return false;
    else
      return true;
#else
    h_file = open(file_path.c_str(), O_RDWR | O_CREAT, 0666); // open or create lockfile
    if (h_file < 0)
      return false;
    //check open success...
    int rc = flock(h_file, LOCK_EX | LOCK_NB); // grab exclusive lock, fail if can't obtain.
    if (rc < 0)
    {
      return false;
    }
    return true;
#endif
  }

  inline bool unlock_and_close_file(native_filesystem_handle& h_file)
  {
#ifdef WIN32
    ::CloseHandle(h_file);                  // no attr. template
#else
    flock(h_file, LOCK_UN); // grab exclusive lock, fail if can't obtain.
    close(h_file);
#endif
    return true;
  }


  inline
    bool load_last_n_from_file_to_string(const std::string& path_to_file, uint64_t size_to_load, std::string& target_str)
  {
      try
      {
        std::ifstream fstream;
        //fstream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fstream.open(path_to_file, std::ios_base::binary | std::ios_base::in | std::ios::ate);
        if (!fstream.good())
          return false;
        std::ifstream::pos_type file_size = fstream.tellg();
         
        uint64_t size_to_load_to_buff = file_size;
        if (static_cast<uint64_t>(file_size) >  size_to_load)
        {
          fstream.seekg(size_to_load_to_buff - size_to_load, std::ios::beg);
          size_to_load_to_buff = size_to_load;
        }else 
        {
          fstream.seekg(0, std::ios::beg);
        }

        size_t size_to_load_to_buff_t = static_cast<size_t>(size_to_load_to_buff);

        target_str.resize(size_to_load_to_buff_t);


        fstream.read((char*)target_str.data(), target_str.size());
        if (!fstream.good())
          return false;

        fstream.close();
        return true;
      }

      catch (...)
      {
        return false;
      }
    }

  inline
  bool copy_file(const std::string& source, const std::string& destination)
  {
    boost::system::error_code ec;
    boost::filesystem::copy_file(epee::string_encoding::utf8_to_wstring(source), epee::string_encoding::utf8_to_wstring(destination), ec);
    if (ec)
      return false;
    else
      return true;
  }

	inline
		bool append_string_to_file(const std::string& path_to_file, const std::string& str)
	{
		try
		{
			boost::filesystem::ofstream fstream;
			fstream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
			fstream.open(epee::string_encoding::utf8_to_wstring(path_to_file), std::ios_base::binary | std::ios_base::out | std::ios_base::app);
			fstream << str;
			fstream.close();
			return true;
		}

		catch(...)
		{
			return false;
		}
	}

	/*
	bool remove_dir_and_subirs(const char* path_to_dir);

	inline 
		bool clean_dir(const char* path_to_dir)
	{
		if(!path_to_dir)
			return false;

		std::string folder = path_to_dir;
		WIN32_FIND_DATAA find_data = {0};
		HANDLE hfind = ::FindFirstFileA((folder + "\\*.*").c_str(), &find_data);
		if(INVALID_HANDLE_VALUE == hfind)
			return false;
		do{
			if(!strcmp("..", find_data.cFileName) || (!strcmp(".", find_data.cFileName)))
				continue;

			if(find_data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
			{
				if(!remove_dir_and_subirs((folder + "\\" + find_data.cFileName).c_str()))
					return false;
			}else
			{
				if(!::DeleteFileA((folder + "\\" + find_data.cFileName).c_str()))
					return false;
			}


		}while(::FindNextFileA(hfind, &find_data));
		::FindClose(hfind);

		return true;
	}
	*/
#ifdef WINDOWS_PLATFORM
	inline bool get_folder_content(const std::string& path, std::list<WIN32_FIND_DATAA>& OUT target_list)
	{
		WIN32_FIND_DATAA find_data = {0};
		HANDLE hfind = ::FindFirstFileA((path + "\\*.*").c_str(), &find_data);
		if(INVALID_HANDLE_VALUE == hfind)
			return false;
		do{
			if(!strcmp("..", find_data.cFileName) || (!strcmp(".", find_data.cFileName)))
				continue;

			target_list.push_back(find_data);

		}while(::FindNextFileA(hfind, &find_data));
		::FindClose(hfind);

		return true;
	}
#endif
	inline bool get_folder_content(const std::string& path, std::list<std::string>& OUT target_list, bool only_files = false)
	{
		try
		{

			boost::filesystem::directory_iterator end_itr; // default construction yields past-the-end
			for ( boost::filesystem::directory_iterator itr( epee::string_encoding::utf8_to_wstring(path) ); itr != end_itr; ++itr )
			{
				if ( only_files && boost::filesystem::is_directory(itr->status()) )
				{
					continue;
				}
				target_list.push_back(itr->path().filename().string());
			}

		}

		catch(...)
		{
			return false;
		}
		return true;
	}
}
}

#endif //_FILE_IO_UTILS_H_
