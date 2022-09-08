
#include "../hdr.hpp"

namespace sdk
{
	inline void* handle = nullptr;
}

namespace sdk
{
	extern auto open_window(const char* window_class) -> void*;

	extern auto get_module_base(const char* name) -> unsigned __int64;

	extern auto patter_scan(unsigned __int64 base, std::string pattern) -> unsigned __int64;

	extern auto get_memory_info(unsigned __int64 address) -> _MEMORY_BASIC_INFORMATION;

	extern auto find_native_class(unsigned __int64 base, const char* asm_module, const char* namespace_name, const char* class_name) -> unsigned __int64;

	template <typename type>
	extern __forceinline auto rvm(unsigned __int64 address, unsigned __int64 size = sizeof(type)) -> type
	{
		type buffer;

		ReadProcessMemory(sdk::handle, (void*)address, &buffer, size, nullptr);

		return buffer;
	}

	template <typename type>
	extern __forceinline auto wvm(unsigned __int64 address, type buffer, unsigned __int64 size = sizeof(type)) -> void
	{
		WriteProcessMemory(sdk::handle, (void*)address, &buffer, size, nullptr);
	}
}

namespace sdk
{
	extern auto get_class_list_from_classloader(void* handle, unsigned __int64 address) -> std::vector<unsigned __int64>;

	extern auto get_class_list_from_sysdict(void* handle, unsigned __int64 address) -> std::vector<unsigned __int64>;

	extern auto find_class(void* handle, std::vector<unsigned __int64> class_list, std::string name) -> unsigned __int64;

	extern auto get_instance(void* handle, unsigned __int64 address) -> unsigned __int64;
}