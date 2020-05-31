#include "model_manager.h"

#include <set>
#include <boost/algorithm/string.hpp>

namespace fs = std::filesystem;

model_loader * model_manager::next() {
    if (current_model == models.size())
        return nullptr;

    if (loaded_models.empty())
        return nullptr;

    if (current_model == models.size())
       return nullptr;

    std::lock_guard<std::mutex> guard(loaded_models_mutex);

    if (current_model > 0) {
	    auto& model = loaded_models[current_model - 1];
	    model->Close();
	    delete model;
	    model = nullptr;
	}

    return loaded_models[current_model++];
}

void model_manager::start()
{
    model_loading_thread = std::thread(&model_manager::load_models, this);
}

void model_manager::load_models() {
    std::cout << "start" << std::endl;

    for (const auto & model_name : models) {

        std::cout << "loading model: " << model_name << std::endl;

        if (stop_loader)
            break;

        auto * model = new model_loader(model_name);
        if (!model->Load(g_hWnd, device, device_context, model_name)) {
            std::string message = "could not load model: ";
            message += model_name;
            throw std::runtime_error(message);
        }

        {
            std::lock_guard<std::mutex> guard(loaded_models_mutex);

            std::cout << "model loaded: " << model_name << std::endl;

            loaded_models.push_back(model);
        }
    }
    std::cout << "end" << std::endl;
}


model_manager::model_manager(
        HWND & g_hWnd,
        ID3D11Device * device,
        ID3D11DeviceContext * device_context
    ) : g_hWnd{ g_hWnd }, device{ device }, device_context{ device_context } {

    auto root_directory = fs::current_path();

    std::set<std::string> extensions_set = { ".obj", ".fbx" };

    for (const auto& p : fs::recursive_directory_iterator(root_directory.append("models")))
    {
        if (p.is_directory())
            continue;

        auto extension = p.path().extension();

        if (extensions_set.find(boost::algorithm::to_lower_copy(extension.string())) != extensions_set.end())
            models.push_back(p.path().string());
    }

    root_directory = fs::current_path().append("output");

    if (!fs::exists(root_directory))
    {
        fs::create_directory(root_directory);
    }

    stop_loader = false;
}



void model_manager::stop() {
    stop_loader = true;
    model_loading_thread.join();
}

model_manager::~model_manager() {
    for(auto & model : loaded_models) {
        if (model) {
            model->Close();
            delete model;
            model = nullptr;
        }
    }
}
