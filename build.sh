#!/bin/bash
#
# cpp-sort One-click Build Script
# Features: dependency download, build, unit tests, performance benchmarks
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_ok() { echo -e "${GREEN}[OK]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Get number of parallel jobs
get_nproc() { nproc 2>/dev/null || echo 4; }

# ============================================================================
# Compile with single-line progress display
# ============================================================================
compile_progress() {
    local prefix="$1"
    stdbuf -oL -eL awk -v prefix="$prefix" -v BLUE='\033[0;34m' -v NC='\033[0m' '
    BEGIN { count = 0 }
    /Building C[XX]+ object/ {
        count++
        match($0, /object [^ ]+/)
        file = substr($0, RSTART+7, RLENGTH-7)
        printf "\r\033[K%s[INFO]%s %s [%d] %s", BLUE, NC, prefix, count, file
        fflush(stdout)
    }
    /Linking/ {
        printf "\r\033[K%s[INFO]%s %s linking...", BLUE, NC, prefix
        fflush(stdout)
    }
    END {
        printf "\r\033[K%s[INFO]%s %s compiled %d files.\n", BLUE, NC, prefix, count
    }
    '
}

# ============================================================================
# Step 1: Download test dependencies (Catch2, RapidCheck)
# ============================================================================
download_dependencies() {
    log_info "Checking test dependencies..."
    
    mkdir -p external
    
    # Download Catch2
    if [ ! -f "external/catch2/CMakeLists.txt" ]; then
        log_info "Downloading Catch2..."
        rm -rf external/catch2
        git clone --depth 1 --branch v3.7.0 https://bgithub.xyz/catchorg/Catch2.git external/catch2
        log_ok "Catch2 downloaded"
    else
        log_info "Catch2 already exists"
    fi
    
    # Download RapidCheck
    if [ ! -f "external/rapidcheck/CMakeLists.txt" ]; then
        log_info "Downloading RapidCheck..."
        rm -rf external/rapidcheck
        git clone --depth 1 https://bgithub.xyz/emil-e/rapidcheck.git external/rapidcheck
        log_ok "RapidCheck downloaded"
    else
        log_info "RapidCheck already exists"
    fi
    
    log_ok "Dependencies ready"
}

# ============================================================================
# Step 2: Build main project
# ============================================================================
build_project() {
    local build_dir="$SCRIPT_DIR/build"
    local nproc=$(get_nproc)
    
    log_info "Building main project..."
    
    # CMake configure
    cmake -S "$SCRIPT_DIR" -B "$build_dir" \
        -DCPPSORT_BUILD_TESTING=ON \
        -DCMAKE_CXX_FLAGS="-Wno-all -Wno-extra -Wno-interference-size" \
        -Wno-dev 2>/dev/null
    
    # Build with progress
    cmake --build "$build_dir" -j"$nproc" -- 2>&1 | compile_progress "Building"
    
    log_ok "Main project built"
}

# ============================================================================
# Step 3: Run unit tests
# ============================================================================
run_unit_tests() {
    local build_dir="$SCRIPT_DIR/build"
    
    log_info "Running unit tests..."
    
    local output
    output=$(cd "$build_dir" && ctest --output-on-failure -j$(get_nproc) 2>&1) || {
        echo "$output"
        log_error "Unit tests failed"
        exit 1
    }
    
    echo "$output"
    
    # Extract stats
    local passed=$(echo "$output" | grep -oP '\d+(?= tests passed)' | tail -1)
    local total=$(echo "$output" | grep -oP '\d+(?= tests? passed)' | tail -1)
    log_ok "Unit tests passed: ${passed}/${total} tests"
}

# ============================================================================
# Step 4: Build performance benchmarks
# ============================================================================
build_benchmarks() {
    local build_dir="$SCRIPT_DIR/benchmarks/sort-comparison/build"
    local nproc=$(get_nproc)
    
    log_info "Building benchmarks..."
    
    cmake -S "$SCRIPT_DIR/benchmarks/sort-comparison" -B "$build_dir" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_FLAGS="-Wno-all -Wno-extra -Wno-interference-size" \
        -Wno-dev 2>/dev/null
    
    cmake --build "$build_dir" -j"$nproc" -- 2>&1 | compile_progress "Building benchmarks"
    
    log_ok "Benchmarks built"
}

# ============================================================================
# Step 5: Run performance benchmarks
# ============================================================================
run_benchmarks() {
    log_info "Running performance benchmarks..."
    "$SCRIPT_DIR/benchmarks/sort-comparison/build/bench-sort-comparison"
    log_ok "Benchmarks complete"
}

# ============================================================================
# Step 6: Run parallel sorters tests
# ============================================================================
run_parallel_tests() {
    local build_dir="$SCRIPT_DIR/benchmarks/sort-comparison/build"
    local nproc=$(get_nproc)
    
    log_info "Running parallel sorters tests..."
    
    # Build if needed
    cmake -S "$SCRIPT_DIR/benchmarks/sort-comparison" -B "$build_dir" \
        -DCMAKE_BUILD_TYPE=Release -Wno-dev 2>/dev/null
    cmake --build "$build_dir" -j"$nproc" -- 2>&1 | compile_progress "Building parallel"
    
    "$build_dir/bench-parallel"
    log_ok "Parallel tests complete"
}

# ============================================================================
# Step 7: Run serial vs parallel comparison
# ============================================================================
run_compare_tests() {
    local algo="${1:-all}"
    local build_dir="$SCRIPT_DIR/benchmarks/sort-comparison/build"
    local nproc=$(get_nproc)
    
    log_info "Running serial vs parallel comparison (${algo})..."
    
    cmake -S "$SCRIPT_DIR/benchmarks/sort-comparison" -B "$build_dir" \
        -DCMAKE_BUILD_TYPE=Release -Wno-dev 2>/dev/null
    cmake --build "$build_dir" -j"$nproc" -- 2>&1 | compile_progress "Building comparison"
    
    if [ "$algo" = "all" ]; then
        "$build_dir/bench-serial-parallel"
    else
        "$build_dir/bench-serial-parallel" -c "$algo"
    fi
    
    log_ok "Comparison complete"
}

# ============================================================================
# Step 8: Run SIMD sorter tests
# ============================================================================
run_simd_tests() {
    log_info "Running SIMD sorter tests..."
    "$SCRIPT_DIR/benchmarks/sort-comparison/build/test-simd-sorter"
    log_ok "SIMD tests complete"
}

# ============================================================================
# Show help
# ============================================================================
show_help() {
    cat << EOF
Usage: $0 [options]

Options:
  -h, --help       Show this help message
  -d, --deps       Download dependencies only
  -b, --build      Build project only
  -t, --test       Run unit tests only
  -B, --bench      Run performance benchmarks only
  -p, --parallel   Run parallel sorters tests
  -s, --simd       Run SIMD sorter tests only
  -c <algo>        Run serial vs parallel comparison
                   Algorithms: all, merge, quick, pdq, tim, heap, grail, simd
  --no-deps        Skip dependency download
  --no-test        Skip unit tests
  --no-bench       Skip performance benchmarks
  --all            Execute all steps (default)

Examples:
  $0               # Execute all steps
  $0 --no-deps     # Skip dependency download
  $0 -b -t         # Build and test only
  $0 -c merge      # Compare merge_sorter vs parallel_merge_sorter
EOF
}

# ============================================================================
# Main
# ============================================================================
main() {
    local run_deps=true
    local run_build=true
    local run_test=true
    local run_bench=true
    local run_parallel=true
    local run_simd=true
    local run_compare=false
    local compare_algo="all"
    
    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)   show_help; exit 0 ;;
            -d|--deps)   run_deps=true; run_build=false; run_test=false; run_bench=false; run_parallel=false; run_simd=false; run_compare=false ;;
            -b|--build)  run_deps=false; run_build=true; run_test=false; run_bench=false; run_parallel=false; run_simd=false; run_compare=false ;;
            -t|--test)   run_deps=false; run_build=false; run_test=true; run_bench=false; run_parallel=false; run_simd=false; run_compare=false ;;
            -B|--bench)  run_deps=false; run_build=false; run_test=false; run_bench=true; run_parallel=false; run_simd=false; run_compare=false ;;
            -p|--parallel) run_deps=false; run_build=false; run_test=false; run_bench=false; run_parallel=true; run_simd=false; run_compare=false ;;
            -s|--simd)   run_deps=false; run_build=false; run_test=false; run_bench=false; run_parallel=false; run_simd=true; run_compare=false ;;
            -c)          run_deps=false; run_build=false; run_test=false; run_bench=false; run_parallel=false; run_simd=false; run_compare=true
                          [[ $# -gt 1 && ! "$2" =~ ^- ]] && { compare_algo="$2"; shift; } ;;
            --no-deps)   run_deps=false ;;
            --no-test)   run_test=false ;;
            --no-bench)  run_bench=false ;;
            --no-parallel) run_parallel=false ;;
            --no-simd)   run_simd=false ;;
            --all)       run_deps=true; run_build=true; run_test=true; run_bench=true; run_parallel=true; run_simd=true; run_compare=false ;;
            *)           log_error "Unknown option: $1"; show_help; exit 1 ;;
        esac
        shift
    done
    
    # Check required tools
    command -v git >/dev/null || { log_error "git not installed"; exit 1; }
    command -v cmake >/dev/null || { log_error "cmake not installed"; exit 1; }
    
    # Start
    log_info "Starting cpp-sort build..."
    
    # Execute steps
    $run_deps && download_dependencies
    $run_build && build_project
    $run_test && run_unit_tests
    $run_parallel && run_parallel_tests
    $run_compare && run_compare_tests "$compare_algo"
    ($run_bench || $run_simd) && build_benchmarks
    $run_simd && run_simd_tests
    $run_bench && run_benchmarks
    
    echo ""
    log_ok "All tasks completed!"
}

main "$@"
