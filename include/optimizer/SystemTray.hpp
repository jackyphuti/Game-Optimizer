#pragma once

#include "optimizer/OptimizerEngine.hpp"
#include <memory>
#include <string>

namespace optimizer {

class SystemTray {
public:
    virtual ~SystemTray() = default;

    virtual bool init(std::shared_ptr<OptimizerEngine> engine) = 0;
    virtual void showNotification(const std::string& title, const std::string& message) = 0;
    virtual void runMessageLoop() = 0;
    virtual void quit() = 0;

    static std::unique_ptr<SystemTray> create(std::shared_ptr<OptimizerEngine> engine);
};

} // namespace optimizer
