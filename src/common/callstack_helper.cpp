// Copyright (c) 2019 Zano Project
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if defined(WIN32)

#define WIN32_LEAN_AND_MEAN 1
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <Psapi.h>
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "dbghelp.lib")

#pragma pack( push, before_imagehlp, 8 )
#include <imagehlp.h>
#pragma pack( pop, before_imagehlp )

#include "include_base_utils.h"
#include "callstack_helper.h"

namespace
{
  struct module_data
  {
    std::string image_name;
    std::string module_name;
    void *base_address;
    DWORD load_size;
  };


  class get_mod_info
  {
    HANDLE process;
    static const int buffer_length = 4096;
  public:
    get_mod_info(HANDLE h) : process(h) {}

    module_data operator()(HMODULE module)
    {
      module_data ret;
      char temp[buffer_length];
      MODULEINFO mi;

      GetModuleInformation(process, module, &mi, sizeof(mi));
      ret.base_address = mi.lpBaseOfDll;
      ret.load_size = mi.SizeOfImage;

      GetModuleFileNameEx(process, module, temp, sizeof(temp));
      ret.image_name = temp;
      GetModuleBaseName(process, module, temp, sizeof(temp));
      ret.module_name = temp;
      std::vector<char> img(ret.image_name.begin(), ret.image_name.end());
      std::vector<char> mod(ret.module_name.begin(), ret.module_name.end());
      SymLoadModule64(process, 0, &img[0], &mod[0], (DWORD64)ret.base_address, ret.load_size);
      return ret;
    }
  };

  std::string get_symbol_undecorated_name(HANDLE process, DWORD64 address, std::stringstream& ss)
  {
    SYMBOL_INFO* sym;
    static const int max_name_len = 1024;
    std::vector<char> sym_buffer(sizeof(SYMBOL_INFO) + max_name_len, '\0');
    sym = (SYMBOL_INFO *)sym_buffer.data();
    sym->SizeOfStruct = sizeof(SYMBOL_INFO);
    sym->MaxNameLen = max_name_len;

    DWORD64 displacement;
    if (!SymFromAddr(process, address, &displacement, sym))
      return std::string("SymFromAddr failed1: ") + epee::string_tools::num_to_string_fast(GetLastError());

    if (*sym->Name == '\0')
      return std::string("SymFromAddr failed2: ") + epee::string_tools::num_to_string_fast(GetLastError());

    /*
    ss << "  SizeOfStruct : " << sym->SizeOfStruct << ENDL;
    ss << "  TypeIndex    : " << sym->TypeIndex << ENDL;        // Type Index of symbol
    ss << "  Index        : " << sym->Index << ENDL;
    ss << "  Size         : " << sym->Size << ENDL;
    ss << "  ModBase      : " << sym->ModBase << ENDL;          // Base Address of module comtaining this symbol
    ss << "  Flags        : " << sym->Flags << ENDL;
    ss << "  Value        : " << sym->Value << ENDL;            // Value of symbol, ValuePresent should be 1
    ss << "  Address      : " << sym->Address << ENDL;          // Address of symbol including base address of module
    ss << "  Register     : " << sym->Register << ENDL;         // register holding value or pointer to value
    ss << "  Scope        : " << sym->Scope << ENDL;            // scope of the symbol
    ss << "  Tag          : " << sym->Tag << ENDL;              // pdb classification
    ss << "  NameLen      : " << sym->NameLen << ENDL;          // Actual length of name
    ss << "  MaxNameLen   : " << sym->MaxNameLen << ENDL;
    ss << "  Name[1]      : " << &sym->Name << ENDL;            // Name of symbol
    */

    std::string und_name(max_name_len, '\0');
    DWORD chars_written = UnDecorateSymbolName(sym->Name, &und_name.front(), max_name_len, UNDNAME_COMPLETE);
    und_name.resize(chars_written);
    return und_name;
  }

} // namespace

namespace tools
{

  std::string get_callstack_win_x64()
  {
    // @TODO@
    // static epee::static_helpers::wrapper<std::recursive_mutex> cs;
    static std::recursive_mutex cs;
    std::lock_guard<std::recursive_mutex> lock(cs);

    HANDLE h_process = GetCurrentProcess();
    HANDLE h_thread = GetCurrentThread();

    PCSTR user_search_path = NULL; // may be path to a pdb?
    if (!SymInitialize(h_process, user_search_path, false))
      return "<win callstack error 1>";

    DWORD sym_options = SymGetOptions();
    sym_options |= SYMOPT_LOAD_LINES | SYMOPT_UNDNAME;
    SymSetOptions(sym_options);

    DWORD cb_needed;
    std::vector<HMODULE> module_handles(1);
    EnumProcessModules(h_process, &module_handles[0], static_cast<DWORD>(module_handles.size() * sizeof(HMODULE)), &cb_needed);
    module_handles.resize(cb_needed / sizeof(HMODULE));
    EnumProcessModules(h_process, &module_handles[0], static_cast<DWORD>(module_handles.size() * sizeof(HMODULE)), &cb_needed);

    std::vector<module_data> modules;
    std::transform(module_handles.begin(), module_handles.end(), std::back_inserter(modules), get_mod_info(h_process));
    void *base = modules[0].base_address;

    CONTEXT context;
    memset(&context, 0, sizeof context);
    RtlCaptureContext( &context );

    STACKFRAME64 frame;
    memset(&frame, 0, sizeof frame);
#ifndef _M_ARM64
    frame.AddrPC.Offset = context.Rip;
#endif 
    frame.AddrPC.Mode = AddrModeFlat;
#ifndef _M_ARM64
    frame.AddrStack.Offset = context.Rsp;
#endif
    frame.AddrStack.Mode = AddrModeFlat;
#ifndef _M_ARM64
    frame.AddrFrame.Offset = context.Rbp;
#endif
    frame.AddrFrame.Mode = AddrModeFlat;
  
    IMAGEHLP_LINE64 line = { 0 };
    line.SizeOfStruct = sizeof line;
    IMAGE_NT_HEADERS *image_nt_header = ImageNtHeader(base);

    std::stringstream ss;
    ss << ENDL;
    // ss << "main module loaded at 0x" << std::hex << std::setw(16) << std::setfill('0') << base << std::dec << " from " << modules[0].image_name << ENDL;
    for (size_t n = 0; n < 250; ++n)
    {
      if (!StackWalk64(image_nt_header->FileHeader.Machine, h_process, h_thread, &frame, &context, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL))
        break;
      if (frame.AddrReturn.Offset == 0)
        break;

      std::string fnName = get_symbol_undecorated_name(h_process, frame.AddrPC.Offset, ss);
      ss << "0x" << std::setw(16) << std::setfill('0') << std::hex << frame.AddrPC.Offset << " " << std::dec << fnName;
      DWORD offset_from_line = 0;
      if (SymGetLineFromAddr64(h_process, frame.AddrPC.Offset, &offset_from_line, &line))
        ss << "+" << offset_from_line << " " << line.FileName << "(" << line.LineNumber << ")";

      for (auto el : modules)
      {
        if ((DWORD64)el.base_address <= frame.AddrPC.Offset && frame.AddrPC.Offset < (DWORD64)el.base_address + (DWORD64)el.load_size)
        {
          ss << " : " << el.module_name << " @ 0x" << std::setw(0) << std::hex << (DWORD64)el.base_address << ENDL;
          break;
        }
      }

    }
    SymCleanup(h_process);

    return ss.str();
  }

} // namespace tools

#endif // #if defined(WIN32)
