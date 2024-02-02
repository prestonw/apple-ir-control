CCFLAGS := -g -std=c++11 -Wall -framework CoreFoundation -framework IOKit

ifndef DEBUG
	CCFLAGS += -DNDEBUG
endif

apple-ir-control: apple-ir-control.cc
	clang++ -o $@ $(CCFLAGS) $<
