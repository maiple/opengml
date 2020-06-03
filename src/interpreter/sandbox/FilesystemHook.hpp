#pragma once

#include <array>

#ifdef OGM_FS_HOOK

namespace ogm::interpreter
{

    // returns true if hooking is implemented on this platform.
    bool can_hook();

    void hook_fs_open();

    // adapted from https://github.com/lapinozz/Vaccinator
    class FunctionHook
    {
    public:
    	FunctionHook(const std::string& targetName);

        // initializes and calls hook.
    	void init(void* hookFunction);

    	void hook();

    	void unhook();

    	template <typename FunctionType, typename...Args>
    	auto callOriginal(Args&&...args)
    	{
    		unhook();
    		auto result = reinterpret_cast<FunctionType>(targetFunction)(std::forward<Args>(args)...);
    		hook();

    		return result;
    	}

    private:
        void unprotect();
        void protect();

    	using RawCode = std::vector<uint8_t>;

    	static RawCode generateShellcode(uintptr_t hook_pointer);

    	void* targetFunction;
    	std::string targetName;
    	RawCode originalCode;
    	RawCode shellcode;

        // used by protect() and unprotect()
        uint64_t old_protect;
    };

}

#endif
