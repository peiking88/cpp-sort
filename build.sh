#!/bin/bash
#
# cpp-sort One-click Build Script
# Features: dependency download, build, unit tests, performance benchmarks
#

set -e  # Exit on error

# Color definitions
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Project root directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="${SCRIPT_DIR}"

# Print colored messages
print_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
print_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
print_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Print separator
print_separator() {
    echo "============================================================"
}

# Check if command exists
check_command() {
    if ! command -v "$1" &> /dev/null; then
        print_error "$1 not installed, please install $1 first"
        exit 1
    fi
}

# Download test dependencies (Catch2, RapidCheck)
download_dependencies() {
    print_separator
    print_info "Checking test dependencies..."
    
    cd "${PROJECT_ROOT}"
    
    # Create external directory
    if [ ! -d "external" ]; then
        mkdir -p external
    fi
    
    # Download Catch2 (for unit tests)
    if [ ! -d "external/catch2" ] || [ ! -f "external/catch2/CMakeLists.txt" ]; then
        print_info "Downloading Catch2..."
        rm -rf external/catch2
        git clone --depth 1 --branch v3.7.0 https://bgithub.xyz/catchorg/Catch2.git external/catch2
        print_success "Catch2 downloaded"
    else
        print_info "Catch2 already exists, skipping"
    fi
    
    # Download RapidCheck (for property-based tests)
    if [ ! -d "external/rapidcheck" ] || [ ! -f "external/rapidcheck/CMakeLists.txt" ]; then
        print_info "Downloading RapidCheck..."
        rm -rf external/rapidcheck
        git clone --depth 1 https://bgithub.xyz/emil-e/rapidcheck.git external/rapidcheck
        print_success "RapidCheck downloaded"
    else
        print_info "RapidCheck already exists, skipping"
    fi
    
    print_success "Dependencies check complete"
}

# Build main project
build_project() {
    print_separator
    print_info "Building main project..."
    
    cd "${PROJECT_ROOT}"
    
    # Create and enter build directory
    mkdir -p build
    cd build
    
    # CMake configure
    print_info "CMake configuration..."
    cmake .. -Wno-dev -DCPPSORT_BUILD_TESTING=ON -DCMAKE_CXX_FLAGS="-Wno-all -Wno-extra -Wno-interference-size" 2>/dev/null
    
    # Compile
    print_info "Compiling..."
    make -j$(nproc) -s 2>&1 | grep -v "^$" | grep -v "warning:" | grep -v "note:" || true
    
    print_success "Main project build complete"
}

# Run unit tests
run_unit_tests() {
    print_separator
    print_info "Running unit tests..."
    
    cd "${PROJECT_ROOT}/build"
    
    # Run tests
    local test_output=$(ctest --output-on-failure -j$(nproc) 2>&1)
    local test_result=$?
    
    echo "$test_output"
    
    if [ $test_result -eq 0 ]; then
        # Extract test statistics
        local passed=$(echo "$test_output" | grep -oP '\d+(?= tests passed)' | tail -1)
        local total=$(echo "$test_output" | grep -oP '\d+(?= tests? passed)' | tail -1)
        print_success "Unit tests passed: ${passed}/${total} tests"
    else
        print_error "Unit tests failed"
        exit 1
    fi
}

# Build performance benchmarks
build_benchmarks() {
    print_separator
    print_info "Building performance benchmarks..."
    
    cd "${PROJECT_ROOT}/benchmarks/sort-comparison"
    
    # Create and enter build directory
    mkdir -p build
    cd build
    
    # CMake configure
    cmake .. -Wno-dev -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-Wno-all -Wno-extra -Wno-interference-size" 2>/dev/null
    
    # Compile
    make -j$(nproc) -s 2>&1 | grep -v "^$" | grep -v "warning:" | grep -v "note:" || true
    
    print_success "Performance benchmarks build complete"
}

# Run performance benchmarks
run_benchmarks() {
    print_separator
    print_info "Running performance benchmarks..."
    
    cd "${PROJECT_ROOT}/benchmarks/sort-comparison/build"
    
    # Run benchmark
    ./bench-sort-comparison
    
    print_success "Performance benchmarks complete"
}

# Run parallel sorters tests (parallel_sorter, parallel_merge_sorter, parallel_quick_sorter, parallel_pdq_sorter)
run_parallel_tests() {
    print_separator
    print_info "Running parallel sorters tests..."
    
    # Build benchmark first
    cd "${PROJECT_ROOT}/benchmarks/sort-comparison"
    mkdir -p build
    cd build
    cmake .. -Wno-dev -DCMAKE_BUILD_TYPE=Release 2>/dev/null
    make -j$(nproc) -s bench-parallel 2>&1 | grep -v "^$" | grep -v "warning:" | grep -v "note:" || true
    
    # Run parallel sorters benchmark
    ./bench-parallel
    
    print_success "Parallel sorters tests complete"
}

# Run serial vs parallel comparison tests
# Usage: run_compare_tests [algorithm]
# algorithm: all, merge, quick, pdq, tim, heap, grail, simd (default: all)
run_compare_tests() {
    local algo="${1:-all}"
    
    print_separator
    print_info "Running serial vs parallel comparison tests (algorithm: ${algo})..."
    
    # Build benchmark first
    cd "${PROJECT_ROOT}/benchmarks/sort-comparison"
    mkdir -p build
    cd build
    cmake .. -Wno-dev -DCMAKE_BUILD_TYPE=Release 2>/dev/null
    make -j$(nproc) -s bench-serial-parallel 2>&1 | grep -v "^$" | grep -v "warning:" | grep -v "note:" || true
    
    # Run serial vs parallel comparison benchmark
    if [ "$algo" = "all" ]; then
        ./bench-serial-parallel
    else
        ./bench-serial-parallel -c "$algo"
    fi
    
    print_success "Serial vs parallel comparison tests complete"
}

# Run simd_sorter tests
run_simd_tests() {
    print_separator
    print_info "Running simd_sorter tests..."
    
    cd "${PROJECT_ROOT}/benchmarks/sort-comparison/build"
    
    # Run simd_sorter tests
    ./test-simd-sorter
    
    print_success "simd_sorter tests complete"
}

# Show help
show_help() {
    echo "Usage: $0 [options]"
    echo ""
    echo "Options:"
    echo "  -h, --help       Show this help message"
    echo "  -d, --deps       Download dependencies only"
    echo "  -b, --build      Build project only"
    echo "  -t, --test       Run unit tests only"
    echo "  -B, --bench      Run performance benchmarks only"
    echo "  -p, --parallel   Run parallel sorters tests (parallel_sorter, parallel_merge_sorter, parallel_quick_sorter, parallel_pdq_sorter)"
    echo "  -s, --simd       Run simd_sorter tests only"
    echo "  -c <algo>        Run serial vs parallel comparison tests for specific algorithm"
    echo "                   Algorithms: all, merge, quick, pdq, tim, heap, grail, simd"
    echo "                   Default: all (compare all algorithm pairs)"
    echo "  --no-deps        Skip dependency download"
    echo "  --no-test        Skip unit tests"
    echo "  --no-bench       Skip performance benchmarks"
    echo "  --all            Execute all steps (default)"
    echo ""
    echo "Examples:"
    echo "  $0               # Execute all steps"
    echo "  $0 --no-deps     # Skip dependency download"
    echo "  $0 -b -t         # Build and test only"
    echo "  $0 -p            # Run parallel_sorter tests only"
    echo "  $0 -s            # Run simd_sorter tests only"
    echo "  $0 -c merge      # Compare merge_sorter vs parallel_merge_sorter"
    echo "  $0 -c all        # Compare all serial vs parallel algorithm pairs"
}

# Main function
main() {
    local run_deps=true
    local run_build=true
    local run_test=true
    local run_perf=true
    local run_parallel=true
    local run_simd=true
    local run_compare=false
    local compare_algo="all"
    
    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -d|--deps)
                run_deps=true
                run_build=false
                run_test=false
                run_perf=false
                run_parallel=false
                run_simd=false
                run_compare=false
                shift
                ;;
            -b|--build)
                run_deps=false
                run_build=true
                run_test=false
                run_perf=false
                run_parallel=false
                run_simd=false
                run_compare=false
                shift
                ;;
            -t|--test)
                run_deps=false
                run_build=false
                run_test=true
                run_perf=false
                run_parallel=false
                run_simd=false
                run_compare=false
                shift
                ;;
            -B|--bench)
                run_deps=false
                run_build=false
                run_test=false
                run_perf=true
                run_parallel=false
                run_simd=false
                run_compare=false
                shift
                ;;
            -p|--parallel)
                run_deps=false
                run_build=false
                run_test=false
                run_perf=false
                run_parallel=true
                run_simd=false
                run_compare=false
                shift
                ;;
            -s|--simd)
                run_deps=false
                run_build=false
                run_test=false
                run_perf=false
                run_parallel=false
                run_simd=true
                run_compare=false
                shift
                ;;
            -c)
                run_deps=false
                run_build=false
                run_test=false
                run_perf=false
                run_parallel=false
                run_simd=false
                run_compare=true
                if [[ $# -gt 1 && ! "$2" =~ ^- ]]; then
                    compare_algo="$2"
                    shift
                fi
                shift
                ;;
            --no-deps)
                run_deps=false
                shift
                ;;
            --no-test)
                run_test=false
                shift
                ;;
            --no-bench)
                run_perf=false
                shift
                ;;
            --no-parallel)
                run_parallel=false
                shift
                ;;
            --no-simd)
                run_simd=false
                shift
                ;;
            --all)
                run_deps=true
                run_build=true
                run_test=true
                run_perf=true
                run_parallel=true
                run_simd=true
                run_compare=false
                shift
                ;;
            *)
                print_error "Unknown option: $1"
                show_help
                exit 1
                ;;
        esac
    done
    
    # Check required tools
    check_command git
    check_command cmake
    check_command make
    
    print_separator
    echo -e "${GREEN}   ____      __           __  ____                  __"
    echo -e "  / ____/___/ /___ ____  / /_/ __ \\____  ____  ____/ /"
    echo -e " / /   / __  / __ \`/ _ \\/ __/ /_/ / __ \\/ __ \\/ __  / "
    echo -e "/ /___/ /_/ / /_/ /  __/ /_/ ____/ /_/ / / / / /_/ /  "
    echo -e "\\____/\\__,_/\\__, /\\___/\\__/_/    \\__,_/_/ /_/\\__,_/   "
    echo -e "            /____/                                     ${NC}"
    print_separator
    echo ""
    
    # Execute steps
    if $run_deps; then
        download_dependencies
    fi
    
    if $run_build; then
        build_project
    fi
    
    if $run_test; then
        run_unit_tests
    fi
    
    if $run_parallel; then
        run_parallel_tests
    fi
    
    if $run_compare; then
        run_compare_tests "$compare_algo"
    fi
    
    if $run_perf || $run_simd; then
        build_benchmarks
    fi
    
    if $run_simd; then
        run_simd_tests
    fi
    
    if $run_perf; then
        run_benchmarks
    fi
    
    print_separator
    print_success "All tasks completed!"
    print_separator
}

# Run main function
main "$@"
