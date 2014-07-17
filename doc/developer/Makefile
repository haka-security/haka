# Configure variables
HAKA_PATH ?= /opt/haka

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
CFLAGS = -Wall -I$(HAKA_PATH)/include/ -I$(SRC_DIR) -fPIC -c -g
LDFLAGS = -L$(HAKA_PATH)/lib/ -lhaka -shared -Wl,-soname,$(MODULE_NAME).ho -Wl,--defsym=luaopen_$(MODULE_NAME)/$(MODULE_NAME)=luaopen_$(MODULE_NAME)
SWIGFLAGS = -I$(HAKA_PATH)/include/ -lua -module $(MODULE_NAME)

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) -o $(BIN) $(OBJS)

install: $(BIN)
	install -g bin -m 555 -o root $(BIN) -D $(HAKA_PATH)/share/haka/modules/$(MODULE_NAME)/$(MODULE_NAME).ho

clean:
	rm -f $(BIN) $(OBJS)

# Haka rules
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -o $@ $<

$(OBJ_DIR)/%_lua.c: $(SRC_DIR)/%.i
	$(SWIG) $(SWIGFLAGS) -o $@ $<

.SECONDARY:
