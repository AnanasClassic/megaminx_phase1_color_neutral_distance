CXX ?= g++
CXXFLAGS ?= -O3 -std=c++20 -Wall -Wextra -Wpedantic -I src

.PHONY: all test quick quick-verify integrity reproduce diameter paper clean
all: test
build:
	mkdir -p build
build/self_test: src/self_test.cpp src/phase1_common.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@
build/verify_csp_independent: src/verify_csp_independent.cpp src/phase1_common.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@
build/generate_phase1_tables: src/generate_phase1_tables.cpp src/phase1_common.hpp | build
	$(CXX) $(CXXFLAGS) -march=x86-64 -mtune=generic $< -o $@
build/verify_coordinate_diameter: src/verify_coordinate_diameter.cpp src/phase1_common.hpp | build
	$(CXX) $(CXXFLAGS) -march=x86-64 -mtune=generic $< -o $@
quick: build/self_test build/verify_csp_independent
	./scripts/quick_verify.sh
test quick-verify: quick
integrity:
	sha256sum --check checksums.sha256
reproduce: build/generate_phase1_tables build/self_test build/verify_csp_independent
	./scripts/reproduce.sh
diameter: build/verify_coordinate_diameter
	./scripts/verify_coordinate_diameter.sh
paper:
	./scripts/build_paper.sh
clean:
	rm -rf build release generated_reproduction generated_quick generated_diameter logs \
		paper/*.aux paper/*.log paper/*.out paper/*.toc paper/*.bbl paper/*.blg \
		paper/*.pdf
