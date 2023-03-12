CC := g++
SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin
EXECUTABLE := $(BIN_DIR)/program

IMGUI_SRC_DIR := libs/imgui
IMGUI_OBJ_DIR := obj/imgui

SRC := $(wildcard $(SRC_DIR)/*.cpp)
OBJ := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC)) $(OBJ_DIR)/glad.o

IMGUI_SRC := $(wildcard $(IMGUI_SRC_DIR)/*.cpp)
IMGUI_OBJ := $(patsubst $(IMGUI_SRC_DIR)/%.cpp, $(IMGUI_OBJ_DIR)/%.o, $(IMGUI_SRC))

CPPFLAGS := -Iinclude `pkg-config --cflags glfw3` -Ilibs/glad/include -Ilibs/imgui --std=c++17
CFLAGS := -Wall -O2
LDFLAGS :=
LDLIBS := `pkg-config --libs glfw3` -ldl -lpthread

all: $(EXECUTABLE)
.PHONY: all

$(EXECUTABLE): $(OBJ) $(IMGUI_OBJ) | $(BIN_DIR)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BIN_DIR):
	mkdir -p $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $@

$(IMGUI_OBJ_DIR):
	mkdir -p $@

$(OBJ_DIR)/glad.o: libs/glad/src/glad.c | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(IMGUI_OBJ_DIR)/%.o: $(IMGUI_SRC_DIR)/%.cpp | $(IMGUI_OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	@$(RM) -rv $(EXECUTABLE) $(OBJ_DIR)

-include $(OBJ:.o=.d)
