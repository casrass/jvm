
#include "sdk.hpp"

auto sdk::open_window(const char* window_class) -> void*
{
	unsigned long pid = 0;

	GetWindowThreadProcessId(FindWindowA(window_class, nullptr), &pid);

	return sdk::handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
}

auto sdk::get_module_base(const char* name) -> unsigned __int64
{
	unsigned long bytes;

	K32EnumProcessModulesEx(sdk::handle, nullptr, 0, &bytes, LIST_MODULES_ALL);

	std::vector<HINSTANCE__*> modules(bytes / sizeof(HINSTANCE__*));

	K32EnumProcessModulesEx(sdk::handle, &modules[0], bytes, &bytes, LIST_MODULES_ALL);

	for (const auto& module : modules)
	{
		std::string module_name(256, 0);

		K32GetModuleFileNameExA(sdk::handle, module, &module_name[0], 256);

		if (module_name.find(name) != std::string::npos)
			return (unsigned __int64)module;
	}

	return 0;
}

auto sdk::patter_scan(unsigned __int64 base, std::string pattern) -> unsigned __int64
{
	_MEMORY_BASIC_INFORMATION mbi;

	for (; VirtualQueryEx(sdk::handle, (void*)base, &mbi, sizeof(mbi)); base += mbi.RegionSize)
	{
		if (mbi.Protect != PAGE_READWRITE || mbi.Protect == PAGE_NOACCESS || mbi.Protect & PAGE_GUARD)
			continue;

		if (mbi.State != MEM_COMMIT)
			continue;

		std::string memory(mbi.RegionSize, 0);

		unsigned __int64 bytes_read = 0;

		ReadProcessMemory(sdk::handle, (void*)base, &memory[0], mbi.RegionSize, &bytes_read);

		memory.resize(bytes_read);

		auto pos = memory.find(pattern);

		if (pos != std::string::npos) return base + pos;
	}

	return std::string::npos;
}

auto sdk::get_memory_info(unsigned __int64 address) ->_MEMORY_BASIC_INFORMATION
{
	_MEMORY_BASIC_INFORMATION mbi;

	VirtualQueryEx(sdk::handle, (void*)address, &mbi, sizeof(mbi));

	return mbi;
}

auto sdk::find_native_class(unsigned __int64 base, const char* asm_module, const char* namespace_name, const char* class_name) -> unsigned __int64
{
	auto dos = sdk::rvm<_IMAGE_DOS_HEADER>(base);

	auto head = sdk::rvm<_IMAGE_SECTION_HEADER>(base + dos.e_lfanew + sizeof(_IMAGE_NT_HEADERS64) + 0x50);

	auto next = sdk::rvm<_IMAGE_SECTION_HEADER>(base + dos.e_lfanew + sizeof(_IMAGE_NT_HEADERS64) + 0x78);

	auto size = next.VirtualAddress - head.VirtualAddress;

	if (strncmp((const char*)head.Name, ".data", 256))
		return 0;

	for (unsigned __int64 offset = size; offset > 0; offset -= 0x8)
	{
		std::string c_name(255, 0), c_namespace(255, 0);

		auto cl = sdk::rvm<unsigned __int64>(base + head.VirtualAddress + offset);

		if (cl != 0)
		{
			auto name_ptr = sdk::rvm<unsigned __int64>(cl + 0x10);

			auto namespace_ptr = sdk::rvm<unsigned __int64>(cl + 0x18);

			if (name_ptr != 0 && namespace_ptr != 0)
			{
				ReadProcessMemory(sdk::handle, (void*)name_ptr, &c_name[0], 255, nullptr);

				ReadProcessMemory(sdk::handle, (void*)namespace_ptr, &c_namespace[0], 255, nullptr);

				if (c_name.find(class_name) == std::string::npos)
					continue;

				if (c_namespace.find(namespace_name) == std::string::npos)
					continue;

				return cl;
			}
		}
	}
}

auto sdk::get_class_list_from_classloader(void* handle, unsigned __int64 address) -> std::vector<unsigned __int64>
{
	std::vector<unsigned __int64> classes;

	unsigned __int64 head = 0;

	ReadProcessMemory(handle, (void*)(address + 0x100), &head, 0x8, nullptr);

	for (; head != 0;)
	{
		unsigned __int64 instance = 0;

		ReadProcessMemory(handle, (void*)(head + 0x38), &instance, 0x8, nullptr);

		std::cout << (void*)instance << std::endl;

		/* head + 0x38 - class_loader */

		if (instance != 0) classes.emplace_back(instance);

		ReadProcessMemory(handle, (void*)(head + 0x70), &head, 0x8, nullptr);
	}

	return classes;
}

auto sdk::get_class_list_from_sysdict(void* handle, unsigned __int64 address) -> std::vector<unsigned __int64>
{
	std::vector<unsigned __int64> classes;

	unsigned __int64 dict = 0;

	ReadProcessMemory(handle, (void*)(address + 0x5a), &dict, 0x8, nullptr);

	unsigned __int32 size = 0;

	ReadProcessMemory(handle, (void*)dict, &size, 0x4, nullptr);

	if (size != 0)
	{
		unsigned __int64 base = 0;

		ReadProcessMemory(handle, (void*)(dict + 0x8), &base, 0x8, nullptr);

		for (unsigned __int32 x = 0, s = size; x < s; ++x)
		{
			unsigned __int64 entry = 0;

			ReadProcessMemory(handle, (void*)(base + x * 0x8), &entry, 0x8, nullptr);

			unsigned __int64 class_entry = 0;

			ReadProcessMemory(handle, (void*)(entry + 0x8), &class_entry, 0x8, nullptr);

			for (; class_entry != 0;)
			{
				unsigned __int64 mirror = 0;

				ReadProcessMemory(handle, (void*)(class_entry + 0x10), &mirror, 0x8, nullptr);

				if (mirror != 0) classes.emplace_back(mirror);

				ReadProcessMemory(handle, (void*)(class_entry + 0x8), &class_entry, 0x8, nullptr);
			}
		}
	}

	return classes;
}

auto sdk::find_class(void* handle, std::vector<unsigned __int64> class_list, std::string name) -> unsigned __int64
{
	unsigned __int64 ret_mirror = 0;

	for (const auto& mirror : class_list)
	{
		unsigned __int64 symbol = 0;

		ReadProcessMemory(handle, (void*)(mirror + 0x10), &symbol, 0x8, nullptr);

		if (symbol != 0)
		{
			unsigned short size = 0;

			ReadProcessMemory(handle, (void*)symbol, &size, 0x2, nullptr);

			std::string buf(size, 0);

			ReadProcessMemory(handle, (void*)(symbol + 0x8), &buf[0], size, nullptr);

			if (buf == name) ret_mirror = mirror;
		}
	}

	return ret_mirror;
}

auto sdk::get_instance(void* handle, unsigned __int64 address) -> unsigned __int64
{
	unsigned __int64 static_fields = 0;

	ReadProcessMemory(handle, (void*)(address + 0x68), &static_fields, 0x8, nullptr);

	if (static_fields != 0)
	{
		unsigned __int32 minecraft = 0;

		ReadProcessMemory(handle, (void*)(static_fields + 0x78), &minecraft, 0x4, nullptr);

		return minecraft;
	}

	return 0;
}