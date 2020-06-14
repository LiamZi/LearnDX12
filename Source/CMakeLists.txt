project(SOURCE)

include_directories(
    ${SOLUTION_DIR}/Source/Common
    ${SOLUTION_DIR}/include
)

add_subdirectory(DX12Samples)
add_subdirectory(ThirdPart)
add_subdirectory(Utility)
