import { common } from '@kit.AbilityKit';
import { i18n } from '@kit.LocalizationKit';
import { hilog } from '@kit.PerformanceAnalysisKit';

const DOMAIN = 0x0000;

interface NativeRenderContext {
  initResourceManager: (resMgr: object) => void;
  initFileDir: (fileDIr: string) => void;
  setLanguage: (language: string) => void;
  startGame: () => void;
}

@Entry
@Component
struct Index {
  private xComponentId: string = 'engine_view';
  private xComponentController: XComponentController = new XComponentController();

  build() {
    Stack() {
      XComponent({
        id: this.xComponentId,
        type: 'surface',
        controller: this.xComponentController,
        libraryname: 'engine'
      })
        .onLoad(() => {
          hilog.info(DOMAIN, 'XComponent', 'XComponent loaded!');
          const nativeContext = this.xComponentController.getXComponentContext() as NativeRenderContext | undefined;
          if (nativeContext) {
            const hostContext = this.getUIContext().getHostContext() as common.UIAbilityContext;
            nativeContext.initResourceManager(hostContext.resourceManager);
            nativeContext.initFileDir(hostContext.filesDir);
            nativeContext.setLanguage(i18n.System.getSystemLanguage());
            nativeContext.startGame();
          }
        })
        .onDestroy(() => {
          hilog.info(DOMAIN, 'XComponent', 'XComponent destroyed!');
        })
        .width('100%')
        .height('100%')
    }
    .width('100%')
    .height('100%')
    .backgroundColor(Color.Black)
  }
}