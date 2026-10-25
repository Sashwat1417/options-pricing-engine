# Adds ASan + UBSan to a target when OPE_ENABLE_SANITIZERS is ON.
function(ope_enable_sanitizers target)
    if(NOT OPE_ENABLE_SANITIZERS)
        return()
    endif()

    if(MSVC)
        message(WARNING "OPE_ENABLE_SANITIZERS is not supported with MSVC; ignoring")
        return()
    endif()

    set(flags -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
    target_compile_options(${target} PRIVATE ${flags})
    target_link_options(${target} PRIVATE ${flags})
endfunction()
