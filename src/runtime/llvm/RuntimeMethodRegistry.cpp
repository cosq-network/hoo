#include "RuntimeMethodRegistry.h"

namespace hooc {
namespace runtime {

RuntimeMethodRegistry& RuntimeMethodRegistry::getInstance() {
    static RuntimeMethodRegistry instance;
    return instance;
}

void RuntimeMethodRegistry::registerClass(const RuntimeClassDescriptor& classDesc) {
    classes_[classDesc.className] = &classDesc;
}

const RuntimeClassDescriptor* RuntimeMethodRegistry::findClass(
    const std::string& className) const
{
    auto it = classes_.find(className);
    if (it != classes_.end()) {
        return it->second;
    }
    return nullptr;
}

const RuntimeMethodDescriptor* RuntimeMethodRegistry::findMethod(
    const std::string& className,
    const std::string& methodName) const
{
    const RuntimeClassDescriptor* classDesc = findClass(className);
    if (!classDesc) {
        return nullptr;
    }

    for (size_t i = 0; i < classDesc->methodCount; ++i) {
        if (classDesc->methods[i].hoocMethodName == methodName) {
            return &classDesc->methods[i];
        }
    }

    return nullptr;
}

bool RuntimeMethodRegistry::isRuntimeClass(const std::string& className) const {
    return classes_.find(className) != classes_.end();
}

} // namespace runtime
} // namespace hooc
