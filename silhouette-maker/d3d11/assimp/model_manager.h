#ifndef SILHOUETTE_MODEL_MANAGER_H
#define SILHOUETTE_MODEL_MANAGER_H

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <filesystem>
#include <atomic>

#include "model_loader.h"

class model_manager
{
    size_t current_model = 0;
    std::vector<std::string> models;
    std::vector<model_loader *> loaded_models;

    HWND & g_hWnd;
    ID3D11Device * device;
    ID3D11DeviceContext * device_context;

    std::mutex loaded_models_mutex;
    std::atomic_bool stop_loader = false;

    std::thread model_loading_thread;
public:
    model_manager(HWND & g_hWnd, ID3D11Device * device, ID3D11DeviceContext * device_context);
    ~model_manager();

    model_loader * next();
    bool has_next_model() const
    {
        return current_model != models.size();
    }

    void start();
    void stop();

    void load_models();
};

#endif //SILHOUETTE_MODEL_MANAGER_H
