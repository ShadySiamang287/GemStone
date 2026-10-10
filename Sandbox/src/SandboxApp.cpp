#include <EntryPoint.h>

class SandboxApp : public GEM::Application {
public:
	SandboxApp(HINSTANCE instance) : Application(instance) {
	}
};

GEM::Application* GEM::CreateApplication(HINSTANCE instance) {
	return new SandboxApp(instance);
}