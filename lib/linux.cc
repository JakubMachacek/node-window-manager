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
        std::cout << line << std::endl;
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
    // exports.Set(Napi::String::New(env, "setWindowBounds"), Napi::Function::New(env, setWindowBounds));
    // exports.Set(Napi::String::New(env, "getWindowBounds"), Napi::Function::New(env, getWindowBounds));
    // exports.Set(Napi::String::New(env, "getWindowTitle"), Napi::Function::New(env, getWindowTitle));
    exports.Set(Napi::String::New(env, "initWindow"), Napi::Function::New(env, initWindow));
    exports.Set(Napi::String::New (env, "isWindow"), Napi::Function::New (env, isWindow));
    // exports.Set(Napi::String::New(env, "bringWindowToTop"), Napi::Function::New(env, bringWindowToTop));
    // exports.Set(Napi::String::New(env, "setWindowMinimized"), Napi::Function::New(env, setWindowMinimized));
    // exports.Set(Napi::String::New(env, "setWindowMaximized"), Napi::Function::New(env, setWindowMaximized));

    return exports;
}

NODE_API_MODULE (addon, Init)
