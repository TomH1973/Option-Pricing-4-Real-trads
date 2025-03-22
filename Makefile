# Makefile for Option Pricing Tools
# Handles compilation of all calculator variants

# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -O3
PROFFLAGS = -pg -O2 -std=c99
MATH_LIBS = -lm

# Build type - set to 'profile' for profiling build, default is normal build
BUILD_TYPE ?= normal

# FFT library detection using pkg-config if available
FFTW_CFLAGS := $(shell pkg-config --cflags fftw3 2>/dev/null || echo "-I/usr/include")
FFTW_LIBS := $(shell pkg-config --libs fftw3 2>/dev/null || echo "-lfftw3 -lm")

# Target executables
TARGETS = calculate_iv calculate_iv_v1 calculate_iv_v2 calculate_sv calculate_sv_v2 calculate_sv_v3 calculate_sv_v4

# Default target
all: $(TARGETS)

# Black-Scholes implementations
calculate_iv: calculate_iv.c
	$(CC) $(CFLAGS) -o $@ $< $(MATH_LIBS)

calculate_iv_v1: calculate_iv_v1.c
	$(CC) $(CFLAGS) -o $@ $< $(MATH_LIBS)

calculate_iv_v2: calculate_iv_v2.c
	$(CC) $(CFLAGS) -o $@ $< $(MATH_LIBS)

# Stochastic volatility implementations
calculate_sv: calculate_sv.c
	$(CC) $(CFLAGS) -o $@ $< $(MATH_LIBS)

calculate_sv_v2: calculate_sv_v2.c
	$(CC) $(CFLAGS) -o $@ $< $(MATH_LIBS)

# FFT-based implementations
calculate_sv_v3: calculate_sv_v3.c
	@echo "Checking for FFTW3 library..."
	@if [ ! -f /usr/include/fftw3.h ] && [ ! -f /usr/local/include/fftw3.h ] && ! pkg-config --exists fftw3; then \
		echo "FFTW3 development headers not found."; \
		echo "Please install the FFTW3 development package:"; \
		echo "  Ubuntu/Debian: sudo apt-get install libfftw3-dev"; \
		echo "  Fedora/RHEL:   sudo dnf install fftw-devel"; \
		echo "  macOS:         brew install fftw"; \
		exit 1; \
	fi
	@echo "FFTW3 library found."
	@if [ "$(BUILD_TYPE)" = "profile" ]; then \
		echo "Building with profiling enabled"; \
		$(CC) $(PROFFLAGS) $(FFTW_CFLAGS) -o $@ $< $(FFTW_LIBS); \
	else \
		$(CC) $(CFLAGS) $(FFTW_CFLAGS) -o $@ $< $(FFTW_LIBS); \
	fi
	@if [ ! -L calculate_sv_v3_link ]; then \
		ln -sf calculate_sv_v3 calculate_sv_v3_link; \
		echo "Created symbolic link 'calculate_sv_v3_link'"; \
	fi

calculate_sv_v4: calculate_sv_v4.c
	@echo "Checking for FFTW3 library..."
	@if [ ! -f /usr/include/fftw3.h ] && [ ! -f /usr/local/include/fftw3.h ] && ! pkg-config --exists fftw3; then \
		echo "FFTW3 development headers not found."; \
		echo "Please install the FFTW3 development package:"; \
		echo "  Ubuntu/Debian: sudo apt-get install libfftw3-dev"; \
		echo "  Fedora/RHEL:   sudo dnf install fftw-devel"; \
		echo "  macOS:         brew install fftw"; \
		exit 1; \
	fi
	@echo "FFTW3 library found."
	@if [ "$(BUILD_TYPE)" = "profile" ]; then \
		echo "Building with profiling enabled"; \
		$(CC) $(PROFFLAGS) $(FFTW_CFLAGS) -o $@ $< $(FFTW_LIBS); \
	else \
		$(CC) $(CFLAGS) $(FFTW_CFLAGS) -o $@ $< $(FFTW_LIBS); \
	fi

# Test targets
test_iv: calculate_iv_v2
	@echo "Testing Black-Scholes implementation..."
	./calculate_iv_v2 5.0 100.0 100.0 0.25 0.05 0.02

test_sv: calculate_sv_v2
	@echo "Testing Heston model implementation..."
	./calculate_sv_v2 5.0 100.0 100.0 0.25 0.05 0.02

test_sv_v3: calculate_sv_v3
	@echo "Testing FFT-based Heston implementation (v3)..."
	./calculate_sv_v3 --debug 5.0 100.0 100.0 0.25 0.05 0.02

test_sv_v4: calculate_sv_v4
	@echo "Testing FFT-based Heston implementation (v4) with default settings..."
	./calculate_sv_v4 --debug 5.0 100.0 100.0 0.25 0.05 0.02
	@echo "Testing with custom FFT parameters..."
	./calculate_sv_v4 --debug --fft-n=8192 --alpha=1.75 --eta=0.025 5.0 100.0 100.0 0.25 0.05 0.02

test: test_iv test_sv test_sv_v3 test_sv_v4

# Run comprehensive tests across various parameters
test_range: calculate_sv_v2
	./test_sv_range.sh

# Profile targets
profile_builds: BUILD_TYPE=profile
profile_builds:
	@echo "Building with profiling instrumentation..."
	$(MAKE) calculate_sv_v3
	$(MAKE) calculate_sv_v4
	@echo "Profiling builds complete - run profile_compare.sh to test"

profile_compare: profile_builds
	@echo "Running profile comparison..."
	./profile_compare.sh

benchmark: profile_builds
	@echo "Running FFT parameter benchmark..."
	./benchmark_fft.sh

# Clean up
clean:
	rm -f $(TARGETS) calculate_sv_v3_link *.o gmon.out
	rm -rf /tmp/profile_data

# Install to user's bin directory
install: all
	@mkdir -p $(HOME)/bin
	@cp $(TARGETS) $(HOME)/bin/
	@echo "Installed executables to $(HOME)/bin/"
	@echo "Make sure $(HOME)/bin is in your PATH."

# Help message
help:
	@echo "Option Pricing Tools Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all             - Build all calculator executables"
	@echo "  test            - Run basic tests for all implementations"
	@echo "  test_range      - Run comprehensive parameter tests"
	@echo "  clean           - Remove built executables"
	@echo "  install         - Install executables to ~/bin"
	@echo ""
	@echo "Profile targets:"
	@echo "  profile_builds  - Build all implementations with profiling enabled"
	@echo "  profile_compare - Run profile comparison between v3 and v4"
	@echo "  benchmark       - Run benchmarks with different FFT parameters"
	@echo ""
	@echo "Individual targets:"
	@echo "  calculate_iv, calculate_iv_v1, calculate_iv_v2"
	@echo "  calculate_sv, calculate_sv_v2, calculate_sv_v3, calculate_sv_v4"
	@echo ""
	@echo "Individual tests:"
	@echo "  test_iv, test_sv, test_sv_v3, test_sv_v4"
	@echo ""
	@echo "Build types:"
	@echo "  make BUILD_TYPE=normal  # Default optimized build"
	@echo "  make BUILD_TYPE=profile # Build with profiling instrumentation"

.PHONY: all clean test test_iv test_sv test_sv_v3 test_sv_v4 test_range install help profile_builds profile_compare benchmark