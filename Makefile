CXX      ?= g++
BUILD    ?= build/$(notdir $(CXX))
OVERLAY  ?= overlay

CXXFLAGS ?= -Os
CXXFLAGS += -std=c++17 -Wall -Wextra -Iinclude -DNDEBUG \
            -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -MMD -MP
LDFLAGS  += -static -Wl,--gc-sections -s

INIT_OBJS := $(patsubst %,$(BUILD)/%.o,main fs sys process service control_socket log)
CTL_OBJS  := $(BUILD)/client.o

all: $(BUILD)/potad_init $(BUILD)/potad-ctl

$(BUILD)/potad_init: $(INIT_OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

$(BUILD)/potad-ctl: $(CTL_OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

$(BUILD)/%.o: src/%.cpp | $(BUILD)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILD):
	mkdir -p $@

install: all
	install -D -m 0755 $(BUILD)/potad_init $(OVERLAY)/sbin/potad_init
	install -D -m 0755 $(BUILD)/potad-ctl $(OVERLAY)/usr/bin/potad-ctl

clean:
	rm -rf build

.PHONY: all install clean
-include $(INIT_OBJS:.o=.d) $(CTL_OBJS:.o=.d)