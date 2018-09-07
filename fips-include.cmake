
SET(NROOT ${CMAKE_CURRENT_LIST_DIR})
SET(CODE_ROOT ${CMAKE_CURRENT_LIST_DIR}/code)

option(N_USE_PRECOMPILED_HEADERS "Use precompiled headers" OFF)

if(FIPS_WINDOWS)
	option(N_STATIC_BUILD "Use static runtime in windows builds" ON)
	if(N_STATIC_BUILD)
		add_definitions(-D__N_STATIC_BUILD)
	endif()
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX /fp:fast /GS-")
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /arch:AVX /fp:fast /GS-")
	SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /RTC1 /RTCc")
	SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /RTC1")
elseif(FIPS_LINUX)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive -std=gnu++0x -msse4.2 -march=sandybridge -ffast-math -fPIC -fno-trapping-math -funsafe-math-optimizations -ffinite-math-only -mrecip=all")
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse4.2 -march=sandybridge -ffast-math -fPIC -fno-trapping-math -funsafe-math-optimizations -ffinite-math-only -mrecip=all")
endif()

if(FIPS_WINDOWS)
	option(N_MATH_DIRECTX "Use DirectXMath" ON)
endif()
if(N_MATH_DIRECTX)
	add_definitions(-D__USE_MATH_DIRECTX)
else()	
	add_definitions(-D__USE_VECMATH)
	OPTION(N_USE_AVX "Use AVX instructionset" OFF)	
endif()

add_definitions(-DIL_STATIC_LIB=1)

set(N_QT4 OFF)
set(N_QT5 OFF)
set(DEFQT "N_QT4")
set(N_QT ${DEFQT} CACHE STRING "Qt Version")
set_property(CACHE N_QT PROPERTY STRINGS "N_QT4" "N_QT5")
set(${N_QT} ON)

set(DEF_RENDERER "N_RENDERER_VULKAN")
set(N_RENDERER ${DEF_RENDERER} CACHE STRING "Nebula 3D Render Device")
set_property(CACHE N_RENDERER PROPERTY STRINGS "N_RENDERER_VULKAN" "N_RENDERER_D3D11" "N_RENDERER_OGL4")
set(${N_RENDERER} ON)

if(N_QT5)
	add_definitions(-D__USE_QT5)
endif()

if(N_QT4)
	add_definitions(-D__USE_QT4)
endif()

if(N_RENDERER_VULKAN)
	OPTION(N_VALIDATION "Use Vulkan validation" OFF)
	if (N_VALIDATION)
		add_definitions(-DNEBULAT_VULKAN_VALIDATION)
	endif()
	add_definitions(-DNEBULA_DEFAULT_FRAMESHADER_NAME="vkdebug")
	add_definitions(-D__VULKAN__)
	add_definitions(-DGRAPHICS_IMPLEMENTATION_NAMESPACE=Vulkan)
elseif(N_RENDERER_OGL4)
	add_definitions(-DNEBULA_DEFAULT_FRAMESHADER_NAME="ogl4debug")
	add_definitions(-D__OGL4__)
	add_definitions(-DGRAPHICS_IMPLEMENTATION_NAMESPACE=OpenGL4)
elseif(N_RENDERER_D3D12)
	OPTION(N_VALIDATION "Use D3D12 validation" OFF)
	if (N_VALIDATION)
		add_definitions(-DNEBULAT_D3D12_VALIDATION)
	endif()
	add_definitions(-DNEBULA_DEFAULT_FRAMESHADER_NAME="d3d12debug")
	add_definitions(-D__D3D12__)
	add_definitions(-DGRAPHICS_IMPLEMENTATION_NAMESPACE=D3D12)
elseif(N_RENDERER_METAL)
	OPTION(N_VALIDATION "Use Metal validation" OFF)
	if (N_VALIDATION)
		add_definitions(-DNEBULAT_METAL_VALIDATION)
	endif()
	add_definitions(-DNEBULA_DEFAULT_FRAMESHADER_NAME="metaldebug")
	add_definitions(-D__METAL__)
	add_definitions(-DGRAPHICS_IMPLEMENTATION_NAMESPACE=Metal)
endif()

option(N_BUILD_NVTT "use NVTT" OFF)

option(N_NEBULA_DEBUG_SHADERS "Compile shaders with debug flag" OFF)

macro(add_shaders)    
    if(SHADERC)           
        if(N_NEBULA_DEBUG_SHADERS)
            #set(shader_debug "-debug")
        endif()               
        
        foreach(shd ${ARGN})        
            get_filename_component(basename ${shd} NAME_WE)        
            get_filename_component(foldername ${shd} DIRECTORY)        
            set(output ${EXPORT_DIR}/shaders/${basename}.fxb)           
            add_custom_command(OUTPUT ${output}
                COMMAND ${SHADERC} -i ${shd} -I ${NROOT}/work/shaders -I ${foldername} -o ${EXPORT_DIR} -t shader ${shader_debug}
                MAIN_DEPENDENCY ${shd}
                WORKING_DIRECTORY ${FIPS_PROJECT_DIR}
                COMMENT "Compiling shader ${shd}"
                VERBATIM
                )        
            fips_files(${shd})
            SOURCE_GROUP("res\\shaders" FILES ${shd})        
        endforeach()             
    endif()
endmacro()
    
macro(add_frameshader)
    if(SHADERC)
    foreach(frm ${ARGN})        
            get_filename_component(basename ${frm} NAME)                   
            set(output ${EXPORT_DIR}/frame/${basename})      
MESSAGE(WARNING ${output})            
            add_custom_command(OUTPUT ${output}
                COMMAND ${SHADERC} -i ${frm} -o ${EXPORT_DIR} -t frame
                MAIN_DEPENDENCY ${frm}
                WORKING_DIRECTORY ${FIPS_PROJECT_DIR}
                COMMENT "Compiling frame shader ${frm}"
                VERBATIM
                )        
            fips_files(${frm})
            SOURCE_GROUP("res\\frameshaders" FILES ${frm})
        endforeach()             
    endif()
endmacro()

macro(add_material)
    if(SHADERC)
    foreach(mat ${ARGN})        
            get_filename_component(basename ${mat} NAME)                   
            set(output ${EXPORT_DIR}/materials/${basename})
            add_custom_command(OUTPUT ${output}
                COMMAND ${SHADERC} -i ${mat} -o ${EXPORT_DIR} -t material
                MAIN_DEPENDENCY ${mat}
                WORKING_DIRECTORY ${FIPS_PROJECT_DIR}
                COMMENT "Compiling frame shader ${mat}"
                VERBATIM
                )        
            fips_files(${mat})
            SOURCE_GROUP("res\\materials" FILES ${mat})
        endforeach()             
    endif()
endmacro()

macro(add_nebula_shaders)                
    if(NOT SHADERC)
        MESSAGE(WARNING "Not compiling shaders, ShaderC not found, did you compile nebula-toolkit?")
    else()    
        if(FIPS_WINDOWS)
            get_filename_component(workdir "[HKEY_CURRENT_USER\\SOFTWARE\\gscept\\ToolkitShared;workdir]" ABSOLUTE)
            set(EXPORT_DIR "${workdir}/export_win32")
        endif()
        
        file(GLOB_RECURSE FXH "${NROOT}/work/shaders/vk/*.fxh")        
        SOURCE_GROUP("res\\shaders\\headers" FILES ${FXH})
        fips_files(${FXH})
        file(GLOB_RECURSE FX "${NROOT}/work/shaders/vk/*.fx")    
        foreach(shd ${FX})        
            add_shaders(${shd})
        endforeach()        
        
        file(GLOB_RECURSE FRM "${NROOT}/work/frame/win32/*.json")    
        foreach(shd ${FRM})        
            add_frameshader(${shd})
        endforeach()        
        
         file(GLOB_RECURSE MAT "${NROOT}/work/materials/*.xml")    
        foreach(shd ${MAT})        
            add_material(${shd})
        endforeach()        
        
    endif()
endmacro()
    
