import {Injectable, Injector, ComponentFactoryResolver, EmbeddedViewRef, ApplicationRef, NgZone} from '@angular/core';
import {TranslateService} from '@ngx-translate/core';
import {ModalContainerComponent} from '../directives/modal-container/modal-container.component';

@Injectable()
export class ModalService {

  private components: any[] = [];

  constructor(
    private componentFactoryResolver: ComponentFactoryResolver,
    private appRef: ApplicationRef,
    private injector: Injector,
    private ngZone: NgZone,
    private translate: TranslateService
  ) {}

  prepareModal(type, message) {
    const length = this.components.push(
      this.componentFactoryResolver.resolveComponentFactory(ModalContainerComponent).create(this.injector)
    );

    this.components[length - 1].instance['type'] = type;
    this.components[length - 1].instance['message'] = message.length ? this.translate.instant(message) : '';
    this.components[length - 1].instance['close'].subscribe(() => {
      this.removeModal(length - 1);
    });

    this.ngZone.run(() => {
      this.appendModal(length - 1);
    });
  }

  appendModal(index) {
    this.appRef.attachView(this.components[index].hostView);
    const domElem = (this.components[index].hostView as EmbeddedViewRef<any>).rootNodes[0] as HTMLElement;
    document.body.appendChild(domElem);
  }

  removeModal(index) {
    if (this.components[index]) {
      this.appRef.detachView(this.components[index].hostView);
      this.components[index].destroy();
      this.components.splice(index, 1);
    } else {
      const last = this.components.length - 1;
      this.appRef.detachView(this.components[last].hostView);
      this.components[last].destroy();
      this.components.splice(last, 1);
    }
  }
}
