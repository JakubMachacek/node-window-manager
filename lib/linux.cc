#include <cmath>
#include <cstdio>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <memory>
#include <string>
#include <array>
#include <sstream>
#include <napi.h>
#include <unistd.h>

std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

Napi::Array getWindows (const Napi::CallbackInfo& info) {
    Napi::Env env{ info.Env () };
    auto arr = Napi::Array::New (env);

    std::string result = exec("xdotool search --onlyvisible --name ''");

    int i = 0;
    std::string line;
    std::istringstream stream{result};

    while (std::getline(stream, line)) {
        arr.Set (i, Napi::Number::New (env, std::stoi(line)));
        i++;
    }

    return arr;
}

Napi::Number getActiveWindow (const Napi::CallbackInfo& info) {
    Napi::Env env{ info.Env () };

    std::string winID = exec("xdotool getactivewindow");

    if (winID.length() > 1) {
        // remove newline leftover from console
        if (!winID.empty() && winID[winID.length()-1] == '\n') {
            winID.erase(winID.length()-1);
        }
        return Napi::Number::New (env, std::stoi(winID));
    } else {
        return Napi::Number::New (env, 0);
    }
}

Napi::String getWindowTitle (const Napi::CallbackInfo& info) {
    Napi::Env env{ info.Env () };

    auto idValue = info[0].As<Napi::Number>().Int64Value();

    std::string base = "xdotool getwindowname ";
    std::string value = std::to_string(idValue);
    std::string commandstr = base + value;
    const char *command = commandstr.c_str();

    std::string result = exec(command);
    if (!result.empty() && result[result.length()-1] == '\n') {
        result.erase(result.length()-1);
    }

    std::string title = result;

    return Napi::String::New (env, title);
}

Napi::Boolean bringWindowToTop (const Napi::CallbackInfo& info) {
    Napi::Env env{ info.Env () };
    
    auto idValue = info[0].As<Napi::Number>().Int64Value();

    std::string base = "xdotool windowactivate ";
    std::string value = std::to_string(idValue);
    std::string commandstr = base + value;
    const char *command = commandstr.c_str();

    std::string result = exec(command);

    return Napi::Boolean::New (env, true);
}

Napi::Boolean setWindowMinimized(const Napi::CallbackInfo &info) {
  Napi::Env env{info.Env()};

  auto idValue = info[0].As<Napi::Number>().Int64Value();

  std::string base = "xdotool windowminimize ";
  std::string value = std::to_string(idValue);
  std::string commandstr = base + value;
  const char *command = commandstr.c_str();

  std::string res = exec(command);
  std::cout << res << " is result." << std::endl;

  return Napi::Boolean::New(env, true);
}

Napi::Boolean setWindowMaximized(const Napi::CallbackInfo &info) {
  Napi::Env env{info.Env()};

  auto idValue = info[0].As<Napi::Number>().Int64Value();

  std::string basecmd = "xdotool windowsize ";
  std::string endcmd = " 100% 100%";
  std::string commandstr = basecmd + std::to_string(idValue) + endcmd;
  const char *command = commandstr.c_str();

  std::string res = exec(command);

  return Napi::Boolean::New(env, true);
}

Napi::Object getWindowBounds (const Napi::CallbackInfo& info) {
    Napi::Env env{ info.Env () };

    auto idValue = info[0].As<Napi::Number>().Int64Value();

    Napi::Object bounds{ Napi::Object::New (env) };

    std::string base = "xdotool getwindowgeometry ";
    std::string value = std::to_string(idValue);
    std::string commandstr = base + value;
    const char *command = commandstr.c_str();

    std::string res = exec(command);
    // std::cout << res << std::endl;

    std::string line;
    std::istringstream stream{res};

    if (!std::getline(stream, line)) {
        throw std::runtime_error("There is no geometry data!");
    }

    if (!std::getline(stream, line)) {
        throw std::runtime_error("There is no window position!");
    }

    unsigned long long xCoord = 0, yCoord = 0;

    // In case we need screen in the future, here is code for that
    // std::sscanf(line.c_str(), "\tPosition: %llu,%llu (screen: %llu)", &xCoord, &yCoord, &screenNum);
    if (std::sscanf(line.c_str(), "\tPosition: %llu,%llu", &xCoord, &yCoord) != 2) {
        throw std::runtime_error("Could not read position entry!");
    }
    if (!std::getline(stream, line)) {
        throw std::runtime_error("There is no window geometry!");
    }
    unsigned long long width = 0, height = 0;


    if (std::sscanf(line.c_str(), "\tGeometry: %llux%llu", &width, &height) != 2) {
        throw std::runtime_error("Could not read geometry entry!");
    }

    bounds.Set ("x", xCoord);
    bounds.Set ("y", yCoord);
    bounds.Set ("width", width);
    bounds.Set ("height", height);

    return bounds;
}

Napi::Boolean setWindowBounds (const Napi::CallbackInfo& info) {
    Napi::Env env{ info.Env () };

    auto idValue = info[0].As<Napi::Number>().Int64Value();
    std::string value = std::to_string(idValue);

    Napi::Object bounds{ info[1].As<Napi::Object> () };

    unsigned int xCoord = bounds.Get ("x").ToNumber ();
    unsigned int yCoord = bounds.Get ("y").ToNumber ();
    unsigned int width = bounds.Get ("width").ToNumber ();
    unsigned int height = bounds.Get ("height").ToNumber ();

    std::cout << idValue << std::endl;
    std::cout << xCoord << std::endl;
    std::cout << yCoord << std::endl;
    std::cout << width << std::endl;
    std::cout << height << std::endl;

    std::string base = "xdotool windowsize ";
    std::string commandstr = base + value + " " + std::to_string(width) + " " + std::to_string(height);
    const char *command = commandstr.c_str();

    std::string result = exec(command);
    std::cout << result << std::endl;

    base = "xdotool windowmove ";
    commandstr = base + value + " " + std::to_string(xCoord) + " " + std::to_string(yCoord);
    command = commandstr.c_str();

    result = exec(command);
    std::cout << result << std::endl;

    // auto handle{ getValueFromCallbackData<HWND> (info, 0) };

    // BOOL b{ MoveWindow (handle, bounds.Get ("x").ToNumber (), bounds.Get ("y").ToNumber (),
    //                     bounds.Get ("width").ToNumber (), bounds.Get ("height").ToNumber (), true) };

    return Napi::Boolean::New (env, true);
}

// transforms window ID into process id and process path
Napi::Object initWindow (const Napi::CallbackInfo& info) {
    Napi::Env env{ info.Env () };
    Napi::Object obj{ Napi::Object::New (env) };
    
    auto idValue = info[0].As<Napi::Number>().Int64Value();

    std::string base = "xdotool getwindowpid ";
    std::string value = std::to_string(idValue);
    std::string commandstr = base + value;
    const char *command = commandstr.c_str();

    std::string result = exec(command);
    
    // based on assumption no process id will be single digit
    if (result.length() > 1) {
        // remove newline leftover from console
        if (!result.empty() && result[result.length()-1] == '\n') {
            result.erase(result.length()-1);
        }
        obj.Set ("processId", result);

        std::string basecmd = "readlink /proc/";
        std::string endcmd = "/exe";
        std::string commandstr = basecmd + result + endcmd;
        const char *command = commandstr.c_str();

        std::string res = exec(command);
        if (!res.empty() && res[res.length()-1] == '\n') {
            res.erase(res.length()-1);
        }

        obj.Set ("path", res);
    } else {
        // this will also cause windows to not be validated as windows because of missing path (window.ts)
        obj.Set ("processId", "");
        obj.Set ("path", "");
    }

    return obj;
}

// automatically returns true as I have no idea how to check it in a sensible way
Napi::Boolean isWindow (const Napi::CallbackInfo& info) {
    Napi::Env env{ info.Env () };
    return Napi::Boolean::New (env, true);
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    exports.Set(Napi::String::New(env, "getWindows"), Napi::Function::New(env, getWindows));
    exports.Set(Napi::String::New(env, "getActiveWindow"), Napi::Function::New(env, getActiveWindow));
    exports.Set(Napi::String::New(env, "setWindowBounds"), Napi::Function::New(env, setWindowBounds));
    exports.Set(Napi::String::New(env, "getWindowBounds"), Napi::Function::New(env, getWindowBounds));
    exports.Set(Napi::String::New(env, "getWindowTitle"), Napi::Function::New(env, getWindowTitle));
    exports.Set(Napi::String::New(env, "initWindow"), Napi::Function::New(env, initWindow));
    exports.Set(Napi::String::New (env, "isWindow"), Napi::Function::New (env, isWindow));
    exports.Set(Napi::String::New(env, "bringWindowToTop"), Napi::Function::New(env, bringWindowToTop));
    exports.Set(Napi::String::New(env, "setWindowMinimized"), Napi::Function::New(env, setWindowMinimized));
    exports.Set(Napi::String::New(env, "setWindowMaximized"), Napi::Function::New(env, setWindowMaximized));

    return exports;
}

NODE_API_MODULE (addon, Init)
