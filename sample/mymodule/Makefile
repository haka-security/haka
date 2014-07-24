# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Configure variables
HAKA_PATH ?= /opt/haka
INSTALL ?= install -o root -g bin

MODULE_NAME = mymodule

SRCS = mymodule.c mymodule.i

# Haka specifics
OBJ_DIR = obj
SRC_DIR = src
OBJS := ${SRCS:%.i=%_lua.c}
OBJS := ${OBJS:%.c=$(OBJ_DIR)/%.o}
SRCS := ${addprefix $(SRC_DIR)/, $(SRCS)}
BIN = $(OBJ_DIR)/$(MODULE_NAME).ho

SWIG = swig
CFLAGS = -Wall -I$(HAKA_PATH)/include/ -I$(HAKA_PATH)/include/haka/lua -I$(SRC_DIR) -fPIC -c -g
LDFLAGS = -L$(HAKA_PATH)/lib/ -lhaka -shared -Wl,-soname,$(MODULE_NAME).ho -Wl,--defsym=luaopen_external/$(MODULE_NAME)=luaopen_$(MODULE_NAME)
SWIGFLAGS = -I$(HAKA_PATH)/include/ -lua -module $(MODULE_NAME)

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) -o $(BIN) $(OBJS)

install: $(BIN)
	$(INSTALL) -m 555 $(BIN) -D $(HAKA_PATH)/share/haka/modules/external/$(MODULE_NAME).ho

clean:
	rm -f $(BIN) $(OBJS)

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

# Haka rules
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(OBJ_DIR)
	$(CC) $(CFLAGS) -o $@ $<

$(OBJ_DIR)/%_lua.c: $(SRC_DIR)/%.i $(OBJ_DIR)
	$(SWIG) $(SWIGFLAGS) -o $@ $<

.SECONDARY:
