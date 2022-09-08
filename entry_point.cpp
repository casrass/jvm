
#include "helper/sdk/sdk.hpp"

auto main() -> __int32
{
	auto handle = sdk::open_window("LWJGL");

	if (handle != nullptr)
	{
		auto jvm_base = sdk::get_module_base("jvm.dll");

		auto cl_list = sdk::get_class_list_from_classloader(handle, sdk::rvm<unsigned __int64>(sdk::patter_scan(
			jvm_base, std::string{ 0x31, 0x36, 0x2e, 0x30, 0x00, 0x00 }) + 0x100));

		auto cd_list = sdk::get_class_list_from_sysdict(handle, sdk::patter_scan(jvm_base, 
			std::string{ 0x00, 0x00, 0x44, (char)0xfd }));
	}

	return std::cin.get() != 0;
}