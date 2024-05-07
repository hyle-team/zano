// Copyright (c) 2014-2018 Zano Project
// Copyright (c) 2014-2018 The Louisdor Project
// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "include_base_utils.h"
#include "zlib_helper.h"
using namespace epee;

#include "util.h"
#include "currency_core/currency_config.h"
#include "version.h"

#ifdef WIN32
#include <windows.h>
#include <shlobj.h>
#include <strsafe.h>
#include <lm.h>
#pragma comment(lib, "netapi32.lib")
#else 
#include <sys/utsname.h>
#endif

#include <boost/asio.hpp>

#include "string_coding.h"

namespace tools
{
  std::function<void(void)> signal_handler::m_handler;
  std::function<void(int, void*)>  signal_handler::m_fatal_handler;

#ifdef WIN32

  bool GetWinMajorMinorVersion(DWORD& major, DWORD& minor)
  {
	  bool bRetCode = false;
	  LPBYTE pinfoRawData = 0;
	  if (NERR_Success == NetWkstaGetInfo(NULL, 100, &pinfoRawData))
	  {
		  WKSTA_INFO_100* pworkstationInfo = (WKSTA_INFO_100*)pinfoRawData;
		  major = pworkstationInfo->wki100_ver_major;
		  minor = pworkstationInfo->wki100_ver_minor;
		  ::NetApiBufferFree(pinfoRawData);
		  bRetCode = true;
	  }
	  return bRetCode;
  }


  std::string get_windows_version_display_string()
  {
	  std::string     winver;
	  OSVERSIONINFOEX osver;
	  SYSTEM_INFO     sysInfo;
	  typedef void(__stdcall *GETSYSTEMINFO) (LPSYSTEM_INFO);

#pragma warning (push)
#pragma warning (disable:4996)
		  memset(&osver, 0, sizeof(osver));
	  osver.dwOSVersionInfoSize = sizeof(osver);
	  GetVersionEx((LPOSVERSIONINFO)&osver);
#pragma warning (pop)
	  DWORD major = 0;
	  DWORD minor = 0;
	  if (GetWinMajorMinorVersion(major, minor))
	  {
		  osver.dwMajorVersion = major;
		  osver.dwMinorVersion = minor;
	  }
	  else if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 2)
	  {
		  OSVERSIONINFOEXW osvi;
		  ULONGLONG cm = 0;
		  cm = VerSetConditionMask(cm, VER_MINORVERSION, VER_EQUAL);
		  ZeroMemory(&osvi, sizeof(osvi));
		  osvi.dwOSVersionInfoSize = sizeof(osvi);
		  osvi.dwMinorVersion = 3;
		  if (VerifyVersionInfoW(&osvi, VER_MINORVERSION, cm))
		  {
			  osver.dwMinorVersion = 3;
		  }
	  }

	  GETSYSTEMINFO getSysInfo = (GETSYSTEMINFO)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetNativeSystemInfo");
	  if (getSysInfo == NULL)  getSysInfo = ::GetSystemInfo;
	  getSysInfo(&sysInfo);

	  if (osver.dwMajorVersion == 10 && osver.dwMinorVersion >= 0 && osver.wProductType != VER_NT_WORKSTATION)  winver = "Windows 10 Server";
	  else if (osver.dwMajorVersion == 10 && osver.dwMinorVersion >= 0 && osver.wProductType == VER_NT_WORKSTATION)  winver = "Windows 10";
    else if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 3 && osver.wProductType != VER_NT_WORKSTATION)  winver = "Windows Server 2012 R2";
    else if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 3 && osver.wProductType == VER_NT_WORKSTATION)  winver = "Windows 8.1";
    else if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 2 && osver.wProductType != VER_NT_WORKSTATION)  winver = "Windows Server 2012";
    else if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 2 && osver.wProductType == VER_NT_WORKSTATION)  winver = "Windows 8";
    else if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 1 && osver.wProductType != VER_NT_WORKSTATION)  winver = "Windows Server 2008 R2";
    else if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 1 && osver.wProductType == VER_NT_WORKSTATION)  winver = "Windows 7";
    else if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 0 && osver.wProductType != VER_NT_WORKSTATION)  winver = "Windows Server 2008";
    else if (osver.dwMajorVersion == 6 && osver.dwMinorVersion == 0 && osver.wProductType == VER_NT_WORKSTATION)  winver = "Windows Vista";
    else if (osver.dwMajorVersion == 5 && osver.dwMinorVersion == 2 && osver.wProductType == VER_NT_WORKSTATION
		  &&  sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) 
      winver = "Windows XP x64";
	  else if (osver.dwMajorVersion == 5 && osver.dwMinorVersion == 2)   winver = "Windows Server 2003";
    else if (osver.dwMajorVersion == 5 && osver.dwMinorVersion == 1)   winver = "Windows XP";
    else if (osver.dwMajorVersion == 5 && osver.dwMinorVersion == 0)   winver = "Windows 2000";
    else  winver = "unknown";

	  if (osver.wServicePackMajor != 0)
	  {
		  std::string sp;
		  char buf[128] = { 0 };
		  sp = " Service Pack ";
		  sprintf_s(buf, sizeof(buf), "%hd", osver.wServicePackMajor);
		  sp.append(buf);
		  winver += sp;
	  }

	  if (osver.dwMajorVersion >= 6)
	  {
		  if (sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
			  winver += ", 64-bit";
		  else if (sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
			  winver += ", 32-bit";
	  }


	  winver += "("
		  + std::to_string(osver.dwMajorVersion) + ":"
		  + std::to_string(osver.dwMinorVersion) + ":"
		  + std::to_string(osver.dwBuildNumber) + ":"
		  + std::to_string(osver.dwPlatformId) + ":"
		  + std::to_string(osver.wServicePackMajor) + ":"
		  + std::to_string(osver.wServicePackMinor) + ":"
		  + std::to_string(osver.wSuiteMask) + ":"
		  + std::to_string(osver.wProductType) + ")";

	  return winver;
  }





  std::string get_windows_version_display_string_()
  {
    typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);
    typedef BOOL (WINAPI *PGPI)(DWORD, DWORD, DWORD, DWORD, PDWORD);
#define BUFSIZE 10000

    char pszOS[BUFSIZE] = {0};
    OSVERSIONINFOEX osvi;
    SYSTEM_INFO si;
    PGNSI pGNSI;
    PGPI pGPI;
    BOOL bOsVersionInfoEx;
    DWORD dwType;

    ZeroMemory(&si, sizeof(SYSTEM_INFO));
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));

    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    bOsVersionInfoEx = GetVersionEx((OSVERSIONINFO*) &osvi);

    if(!bOsVersionInfoEx) return pszOS;

    // Call GetNativeSystemInfo if supported or GetSystemInfo otherwise.

    pGNSI = (PGNSI) GetProcAddress(
      GetModuleHandle(TEXT("kernel32.dll")), 
      "GetNativeSystemInfo");
    if(NULL != pGNSI)
      pGNSI(&si);
    else GetSystemInfo(&si);

    if ( VER_PLATFORM_WIN32_NT==osvi.dwPlatformId && 
      osvi.dwMajorVersion > 4 )
    {
      StringCchCopy(pszOS, BUFSIZE, TEXT("Microsoft "));

      // Test for the specific product.

      if ( osvi.dwMajorVersion == 6 )
      {
        if( osvi.dwMinorVersion == 0 )
        {
          if( osvi.wProductType == VER_NT_WORKSTATION )
            StringCchCat(pszOS, BUFSIZE, TEXT("Windows Vista "));
          else StringCchCat(pszOS, BUFSIZE, TEXT("Windows Server 2008 " ));
        }

        if ( osvi.dwMinorVersion == 1 )
        {
          if( osvi.wProductType == VER_NT_WORKSTATION )
            StringCchCat(pszOS, BUFSIZE, TEXT("Windows 7 "));
          else StringCchCat(pszOS, BUFSIZE, TEXT("Windows Server 2008 R2 " ));
        }

        pGPI = (PGPI) GetProcAddress(
          GetModuleHandle(TEXT("kernel32.dll")), 
          "GetProductInfo");

        pGPI( osvi.dwMajorVersion, osvi.dwMinorVersion, 0, 0, &dwType);

        switch( dwType )
        {
        case PRODUCT_ULTIMATE:
          StringCchCat(pszOS, BUFSIZE, TEXT("Ultimate Edition" ));
          break;
        case PRODUCT_PROFESSIONAL:
          StringCchCat(pszOS, BUFSIZE, TEXT("Professional" ));
          break;
        case PRODUCT_HOME_PREMIUM:
          StringCchCat(pszOS, BUFSIZE, TEXT("Home Premium Edition" ));
          break;
        case PRODUCT_HOME_BASIC:
          StringCchCat(pszOS, BUFSIZE, TEXT("Home Basic Edition" ));
          break;
        case PRODUCT_ENTERPRISE:
          StringCchCat(pszOS, BUFSIZE, TEXT("Enterprise Edition" ));
          break;
        case PRODUCT_BUSINESS:
          StringCchCat(pszOS, BUFSIZE, TEXT("Business Edition" ));
          break;
        case PRODUCT_STARTER:
          StringCchCat(pszOS, BUFSIZE, TEXT("Starter Edition" ));
          break;
        case PRODUCT_CLUSTER_SERVER:
          StringCchCat(pszOS, BUFSIZE, TEXT("Cluster Server Edition" ));
          break;
        case PRODUCT_DATACENTER_SERVER:
          StringCchCat(pszOS, BUFSIZE, TEXT("Datacenter Edition" ));
          break;
        case PRODUCT_DATACENTER_SERVER_CORE:
          StringCchCat(pszOS, BUFSIZE, TEXT("Datacenter Edition (core installation)" ));
          break;
        case PRODUCT_ENTERPRISE_SERVER:
          StringCchCat(pszOS, BUFSIZE, TEXT("Enterprise Edition" ));
          break;
        case PRODUCT_ENTERPRISE_SERVER_CORE:
          StringCchCat(pszOS, BUFSIZE, TEXT("Enterprise Edition (core installation)" ));
          break;
        case PRODUCT_ENTERPRISE_SERVER_IA64:
          StringCchCat(pszOS, BUFSIZE, TEXT("Enterprise Edition for Itanium-based Systems" ));
          break;
        case PRODUCT_SMALLBUSINESS_SERVER:
          StringCchCat(pszOS, BUFSIZE, TEXT("Small Business Server" ));
          break;
        case PRODUCT_SMALLBUSINESS_SERVER_PREMIUM:
          StringCchCat(pszOS, BUFSIZE, TEXT("Small Business Server Premium Edition" ));
          break;
        case PRODUCT_STANDARD_SERVER:
          StringCchCat(pszOS, BUFSIZE, TEXT("Standard Edition" ));
          break;
        case PRODUCT_STANDARD_SERVER_CORE:
          StringCchCat(pszOS, BUFSIZE, TEXT("Standard Edition (core installation)" ));
          break;
        case PRODUCT_WEB_SERVER:
          StringCchCat(pszOS, BUFSIZE, TEXT("Web Server Edition" ));
          break;
        }
      }

      if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 )
      {
        if( GetSystemMetrics(SM_SERVERR2) )
          StringCchCat(pszOS, BUFSIZE, TEXT( "Windows Server 2003 R2, "));
        else if ( osvi.wSuiteMask & VER_SUITE_STORAGE_SERVER )
          StringCchCat(pszOS, BUFSIZE, TEXT( "Windows Storage Server 2003"));
        else if ( osvi.wSuiteMask & VER_SUITE_WH_SERVER )
          StringCchCat(pszOS, BUFSIZE, TEXT( "Windows Home Server"));
        else if( osvi.wProductType == VER_NT_WORKSTATION &&
          si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64)
        {
          StringCchCat(pszOS, BUFSIZE, TEXT( "Windows XP Professional x64 Edition"));
        }
        else StringCchCat(pszOS, BUFSIZE, TEXT("Windows Server 2003, "));

        // Test for the server type.
        if ( osvi.wProductType != VER_NT_WORKSTATION )
        {
          if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_IA64 )
          {
            if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
              StringCchCat(pszOS, BUFSIZE, TEXT( "Datacenter Edition for Itanium-based Systems" ));
            else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
              StringCchCat(pszOS, BUFSIZE, TEXT( "Enterprise Edition for Itanium-based Systems" ));
          }

          else if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64 )
          {
            if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
              StringCchCat(pszOS, BUFSIZE, TEXT( "Datacenter x64 Edition" ));
            else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
              StringCchCat(pszOS, BUFSIZE, TEXT( "Enterprise x64 Edition" ));
            else StringCchCat(pszOS, BUFSIZE, TEXT( "Standard x64 Edition" ));
          }

          else
          {
            if ( osvi.wSuiteMask & VER_SUITE_COMPUTE_SERVER )
              StringCchCat(pszOS, BUFSIZE, TEXT( "Compute Cluster Edition" ));
            else if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
              StringCchCat(pszOS, BUFSIZE, TEXT( "Datacenter Edition" ));
            else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
              StringCchCat(pszOS, BUFSIZE, TEXT( "Enterprise Edition" ));
            else if ( osvi.wSuiteMask & VER_SUITE_BLADE )
              StringCchCat(pszOS, BUFSIZE, TEXT( "Web Edition" ));
            else StringCchCat(pszOS, BUFSIZE, TEXT( "Standard Edition" ));
          }
        }
      }

      if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 )
      {
        StringCchCat(pszOS, BUFSIZE, TEXT("Windows XP "));
        if( osvi.wSuiteMask & VER_SUITE_PERSONAL )
          StringCchCat(pszOS, BUFSIZE, TEXT( "Home Edition" ));
        else StringCchCat(pszOS, BUFSIZE, TEXT( "Professional" ));
      }

      if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 )
      {
        StringCchCat(pszOS, BUFSIZE, TEXT("Windows 2000 "));

        if ( osvi.wProductType == VER_NT_WORKSTATION )
        {
          StringCchCat(pszOS, BUFSIZE, TEXT( "Professional" ));
        }
        else 
        {
          if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
            StringCchCat(pszOS, BUFSIZE, TEXT( "Datacenter Server" ));
          else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
            StringCchCat(pszOS, BUFSIZE, TEXT( "Advanced Server" ));
          else StringCchCat(pszOS, BUFSIZE, TEXT( "Server" ));
        }
      }

      // Include service pack (if any) and build number.

      if( strlen(osvi.szCSDVersion) > 0 )
      {
        StringCchCat(pszOS, BUFSIZE, TEXT(" ") );
        StringCchCat(pszOS, BUFSIZE, osvi.szCSDVersion);
      }

      TCHAR buf[80];

      StringCchPrintf( buf, 80, TEXT(" (build %d)"), osvi.dwBuildNumber);
      StringCchCat(pszOS, BUFSIZE, buf);

      if ( osvi.dwMajorVersion >= 6 )
      {
        if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64 )
          StringCchCat(pszOS, BUFSIZE, TEXT( ", 64-bit" ));
        else if (si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_INTEL )
          StringCchCat(pszOS, BUFSIZE, TEXT(", 32-bit"));
      }

      return pszOS; 
    }
    else
    {  
      printf( "This sample does not support this version of Windows.\n");
      return pszOS;
    }
  }

  void signal_handler::GenerateCrashDump(EXCEPTION_POINTERS *pep /* = NULL*/)
  {
    SYSTEMTIME sysTime = { 0 };
    GetSystemTime(&sysTime);
    // get the computer name
    char compName[MAX_COMPUTERNAME_LENGTH + 1] = { 0 };
    DWORD compNameLen = ARRAYSIZE(compName);
    GetComputerNameA(compName, &compNameLen);
    // build the filename: APPNAME_COMPUTERNAME_DATE_TIME.DMP
    char path[MAX_PATH*10] = { 0 };
    std::string folder = epee::log_space::log_singletone::get_default_log_folder();
    sprintf_s(path, ARRAYSIZE(path),"%s\\crashdump_" PROJECT_VERSION_LONG "_%s_%04u-%02u-%02u_%02u-%02u-%02u.dmp",
      folder.c_str(), compName, sysTime.wYear, sysTime.wMonth, sysTime.wDay,
      sysTime.wHour, sysTime.wMinute, sysTime.wSecond);

    HANDLE hFile = CreateFileA(path, GENERIC_READ | GENERIC_WRITE,
      0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if ((hFile != NULL) && (hFile != INVALID_HANDLE_VALUE))
    {
      // Create the minidump 
      MINIDUMP_EXCEPTION_INFORMATION mdei;

      mdei.ThreadId = GetCurrentThreadId();
      mdei.ExceptionPointers = pep;
      mdei.ClientPointers = FALSE;

      MINIDUMP_CALLBACK_INFORMATION mci;

      mci.CallbackRoutine = (MINIDUMP_CALLBACK_ROUTINE)MyMiniDumpCallback;
      mci.CallbackParam = 0;

      MINIDUMP_TYPE mdt = (MINIDUMP_TYPE)(MiniDumpWithPrivateReadWriteMemory |
        MiniDumpWithDataSegs |
        MiniDumpWithHandleData |
        MiniDumpWithFullMemoryInfo |
        MiniDumpWithThreadInfo |
        MiniDumpWithUnloadedModules);

      BOOL rv = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
        hFile, mdt, (pep != 0) ? &mdei : 0, 0, &mci);

      if (!rv)
      {
        LOG_ERROR("Minidump file create FAILED(error " << GetLastError() << ") on path: " <<  path);
      }
      else
      {
        LOG_PRINT_L0("Minidump file created on path: " << path);
      }
      // Close the file 
      CloseHandle(hFile);
    }
    else
    {
      LOG_ERROR("Minidump FAILED to create file (error " << GetLastError() << ") on path: " << path);
    }
  }



#else // ifdef WIN32
std::string get_nix_version_display_string()
{
  utsname un;

  if(uname(&un) < 0)
    return std::string("*nix: failed to get os version");
  return std::string() + un.sysname + " " + un.version + " " + un.release;
}
#endif



  std::string get_os_version_string()
  {
#ifdef WIN32
    return get_windows_version_display_string();
#else
    return get_nix_version_display_string();
#endif
  }



#ifdef WIN32
  std::wstring get_special_folder_path_w(int nfolder, bool iscreate)
  {
    wchar_t psz_path[MAX_PATH] = L"";

    if (SHGetSpecialFolderPathW(NULL, psz_path, nfolder, iscreate))
    {
      return psz_path;
    }

    LOG_ERROR("SHGetSpecialFolderPathW(" << nfolder << ", " << iscreate << ") failed, could not obtain requested path.");
    return L"";
  }

  std::string get_special_folder_path_utf8(int nfolder, bool iscreate)
  {
    return epee::string_encoding::wstring_to_utf8(get_special_folder_path_w(nfolder, iscreate));
  }
#endif

  std::string get_default_data_dir()
  {
    //namespace fs = boost::filesystem;
    // Windows < Vista: C:\Documents and Settings\Username\Application Data\CURRENCY_NAME_SHORT
    // Windows >= Vista: C:\Users\Username\AppData\Roaming\CURRENCY_NAME_SHORT
    // Mac: ~/Library/Application Support/CURRENCY_NAME_SHORT
    // Unix: ~/.CURRENCY_NAME_SHORT
    std::string config_folder;
#ifdef WIN32
    // Windows
#ifdef _M_X64
    config_folder = get_special_folder_path_utf8(CSIDL_APPDATA, true) + "/" + CURRENCY_NAME_SHORT;
#else 
    config_folder = get_special_folder_path_utf8(CSIDL_APPDATA, true) + "/" + CURRENCY_NAME_SHORT + "-x86";
#endif 
#else
    std::string pathRet;
    char* pszHome = getenv("HOME");
    if (pszHome == NULL || strlen(pszHome) == 0)
      pathRet = "/";
    else
      pathRet = pszHome;
#ifdef __APPLE__
    // Mac
    pathRet += "/Library/Application Support";
    config_folder =  (pathRet + "/" + CURRENCY_NAME_SHORT);
#else
    // Unix
    config_folder = (pathRet + "/." + CURRENCY_NAME_SHORT);
#endif
#endif

    return config_folder;
  }
  std::string get_host_computer_name()
  {
    char szname[1024] = "";
    gethostname(szname, sizeof(szname));
    szname[sizeof(szname)-1] = 0; //just to be sure
    return szname;
  }


  std::string get_default_user_dir()
  {
    //namespace fs = boost::filesystem;
    // Windows < Vista: C:\Documents and Settings\Username 
    // Windows >= Vista: C:\Users\Username\AppData\Roaming\CURRENCY_NAME_SHORT
    // Mac: ~/Library/Application Support/CURRENCY_NAME_SHORT
    // Unix: ~/.CURRENCY_NAME_SHORT
    std::string wallets_dir;
#ifdef WIN32
    // Windows
    wallets_dir = get_special_folder_path_utf8(CSIDL_PERSONAL, true) + "/" + CURRENCY_NAME_BASE;
#else
    std::string pathRet;
    char* pszHome = getenv("HOME");
    if (pszHome == NULL || strlen(pszHome) == 0)
      pathRet = "/";
    else
      pathRet = pszHome;

    wallets_dir = (pathRet + "/" + CURRENCY_NAME_BASE);

#endif
    return wallets_dir;
  }

  std::string get_current_username()
  {
#ifdef WIN32
    // Windows
    const char* psz_username = getenv("USERNAME");
#else
    const char* psz_username = getenv("USER");
#endif
    if (psz_username == NULL || strlen(psz_username) == 0)
      psz_username = "unknown_user";

    return std::string(psz_username);
  }



  bool create_directories_if_necessary(const std::string& path)
  {
    namespace fs = boost::filesystem;
    boost::system::error_code ec;
    fs::path fs_path = epee::string_encoding::utf8_to_wstring(path);
    if (fs::is_directory(fs_path, ec))
    {
      return true;
    }

    bool res = fs::create_directories(fs_path, ec);
    if (res)
    {
      LOG_PRINT_L2("Created directory: " << path);
    }
    else
    {
      LOG_PRINT_L2("Can't create directory: " << path << ", err: "<< ec.message());
    }

    return res;
  }

  std::error_code replace_file(const std::string& replacement_name, const std::string& replaced_name)
  {
    int code;
#if defined(WIN32)
    // Maximizing chances for success
    DWORD attributes = ::GetFileAttributes(replaced_name.c_str());
    if (INVALID_FILE_ATTRIBUTES != attributes)
    {
      ::SetFileAttributes(replaced_name.c_str(), attributes & (~FILE_ATTRIBUTE_READONLY));
    }

    bool ok = 0 != ::MoveFileEx(replacement_name.c_str(), replaced_name.c_str(), MOVEFILE_REPLACE_EXISTING);
    code = ok ? 0 : static_cast<int>(::GetLastError());
#else
    bool ok = 0 == std::rename(replacement_name.c_str(), replaced_name.c_str());
    code = ok ? 0 : errno;
#endif
    return std::error_code(code, std::system_category());
  }

#define REQUEST_LOG_CHUNK_SIZE_MAX (10 * 1024 * 1024)

  bool get_log_chunk_gzipped(uint64_t offset, uint64_t size, std::string& output, std::string& error)
  {
    if (size > REQUEST_LOG_CHUNK_SIZE_MAX)
    {
      error = std::string("size is exceeding the limit = ") + epee::string_tools::num_to_string_fast(REQUEST_LOG_CHUNK_SIZE_MAX);
      return false;
    }

    std::string log_filename = epee::log_space::log_singletone::get_actual_log_file_path();
    if (std::ifstream log{ log_filename, std::ifstream::ate | std::ifstream::binary })
    {
      uint64_t file_size = log.tellg();

      if (offset >= file_size)
      {
        error = "offset is out of bounds";
        return false;
      }

      if (offset + size > file_size)
      {
        error = "offset + size if out of bounds";
        return false;
      }

      if (size != 0)
      {
        log.seekg(offset);
        output.resize(size);
        log.read(&output.front(), size);
        uint64_t read_bytes = log.gcount();
        if (read_bytes != size)
        {
          error = std::string("read bytes: ") + epee::string_tools::num_to_string_fast(read_bytes);
          return false;
        }

        if (!epee::zlib_helper::pack(output))
        {
          error = "zlib pack failed";
          return false;
        }
      }

      return true;
    }

    error = std::string("can't open ") + log_filename;
    return false;
  }

  uint64_t get_log_file_size()
  {
    std::string log_filename = epee::log_space::log_singletone::get_actual_log_file_path();
    std::ifstream in(log_filename, std::ifstream::ate | std::ifstream::binary);
    return static_cast<uint64_t>(in.tellg());
  }

  bool check_remote_client_version(const std::string& client_ver)
  {
    std::string v = client_ver.substr(0, client_ver.find('[')); // remove commit id
    v = v.substr(0, v.rfind('.')); // remove build number

    int v_major = 0, v_minor = 0, v_revision = 0;

    size_t dot_pos = v.find('.');
    if (dot_pos == std::string::npos || !epee::string_tools::string_to_num_fast(v.substr(0, dot_pos), v_major))
      return false;

    v = v.substr(dot_pos + 1);
    dot_pos = v.find('.');
    if (!epee::string_tools::string_to_num_fast(v.substr(0, dot_pos), v_minor))
      return false;

    if (dot_pos != std::string::npos)
    {
      // revision
      v = v.substr(dot_pos + 1);
      if (!epee::string_tools::string_to_num_fast(v, v_revision))
        return false;
    }

    // got v_major, v_minor, v_revision

    // allow 2.x and greater
    if (v_major < 2)
      return false;

    return true;
  }

  //this code was taken from https://stackoverflow.com/a/8594696/5566653 
  //credits goes to @nijansen: https://stackoverflow.com/users/1056003/nijansen 
  bool copy_dir( boost::filesystem::path const & source, boost::filesystem::path const & destination)
  {
    namespace fs = boost::filesystem;
    try
    {
      // Check whether the function call is valid
      if (!fs::exists(source) ||!fs::is_directory(source))
      {
        LOG_ERROR("Source directory " << source.string() << " does not exist or is not a directory.");
        return false;
      }
      if (!fs::exists(destination))
      {
        if (!fs::create_directory(destination))
        {
          LOG_ERROR("Unable to create destination directory" << destination.string());
          return false;
        }
      }
      // Create the destination directory
    }
    catch (fs::filesystem_error const & e)
    {
      LOG_ERROR("Exception: " << e.what());
      return false;
    }
    // Iterate through the source directory
    for (fs::directory_iterator file(source); file != fs::directory_iterator(); ++file)
    {
      try
      {
        fs::path current(file->path());
        if (fs::is_directory(current))
        {
          // Found directory: Recursion
          if (!copy_dir(current, destination / current.filename()))
          {
            return false;
          }
        }
        else
        {
          // Found file: Copy
          fs::copy_file( current, destination / current.filename());
        }
      }
      catch (fs::filesystem_error const & e)
      {
        LOG_ERROR("Exception: " << e.what());
      }
    }
    return true;
  }

} // namespace tools
