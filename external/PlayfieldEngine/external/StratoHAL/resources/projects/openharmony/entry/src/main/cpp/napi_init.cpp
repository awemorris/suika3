//
// Screen Config
//
int VIEWPORT_WIDTH = 1280;
int VIEWPORT_HEIGHT = 720;

extern "C" void onLoadSharedObject(void);

//
// DLL Initializer
//
__attribute__((constructor)) void DLLInit(void)
{
    onLoadSharedObject();
}
