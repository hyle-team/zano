import {Directive, Input, ElementRef, HostListener, Renderer2, HostBinding} from '@angular/core';

@Directive({
  selector: '[tooltip]'
})

export class TooltipDirective {

  @HostBinding('style.cursor') cursor = 'pointer';

  @Input('tooltip') tooltipTitle: string;
  @Input() placement: string;
  @Input() tooltipClass: string;
  @Input() delay: number;
  tooltip: HTMLElement;

  removeTooltipTimeout;

  constructor(private el: ElementRef, private renderer: Renderer2) {
  }

  @HostListener('mouseenter') onMouseEnter() {
    if (!this.tooltip) {
      this.show();
    } else {
      this.cancelHide();
    }
  }

  @HostListener('mouseleave') onMouseLeave() {
    if (this.tooltip) {
      this.hide();
    }
  }

  show() {
    this.create();
    this.setPosition();
  }

  hide() {
    this.renderer.setStyle(this.tooltip, 'opacity', '0');
    this.removeTooltipTimeout = setTimeout(() => {
      this.renderer.removeChild(document.body, this.tooltip);
      this.tooltip = null;
    }, this.delay);
  }

  cancelHide() {
    clearTimeout(this.removeTooltipTimeout);
    this.renderer.setStyle(this.tooltip, 'opacity', '1');
  }

  create() {
    this.tooltip = this.renderer.createElement('span');
    this.renderer.appendChild(this.tooltip, this.renderer.createText(this.tooltipTitle));
    this.renderer.appendChild(document.body, this.tooltip);
    this.renderer.setStyle(document.body, 'position', 'relative');
    this.renderer.setStyle(this.tooltip, 'position', 'absolute');
    if (this.tooltipClass !== null) {
      this.renderer.addClass(this.tooltip, this.tooltipClass);
    }
    if (this.placement !== null) {
      this.renderer.addClass(this.tooltip, 'ng-tooltip-' + this.placement);
    } else {
      this.placement = 'top';
      this.renderer.addClass(this.tooltip, 'ng-tooltip-top');
    }
    if (this.delay !== null) {
      this.renderer.setStyle(this.tooltip, 'opacity', '0');
      this.renderer.setStyle(this.tooltip, '-webkit-transition', `opacity ${this.delay}ms`);
      this.renderer.setStyle(this.tooltip, '-moz-transition', `opacity ${this.delay}ms`);
      this.renderer.setStyle(this.tooltip, '-o-transition', `opacity ${this.delay}ms`);
      this.renderer.setStyle(this.tooltip, 'transition', `opacity ${this.delay}ms`);
      window.setTimeout(() => {
        this.renderer.setStyle(this.tooltip, 'opacity', '1');
      }, 0);
    }
  }

  setPosition() {
    const hostPos = this.el.nativeElement.getBoundingClientRect();
    const tooltipPos = this.tooltip.getBoundingClientRect();
    // const scrollPos = window.pageYOffset || document.documentElement.scrollTop || document.body.scrollTop || 0;

    if (this.placement === 'top') {
      this.renderer.setStyle(this.tooltip, 'top', hostPos.top - tooltipPos.height + 'px');
      this.renderer.setStyle(this.tooltip, 'left', hostPos.left + 'px');
    }

    if (this.placement === 'bottom') {
      if (window.innerHeight < hostPos.bottom + this.tooltip.offsetHeight + parseInt(getComputedStyle(this.tooltip).marginTop, 10)) {
        this.renderer.removeClass(this.tooltip, 'ng-tooltip-bottom');
        this.renderer.addClass(this.tooltip, 'ng-tooltip-top');
        this.renderer.setStyle(this.tooltip, 'top', hostPos.top - tooltipPos.height + 'px');
        this.renderer.setStyle(this.tooltip, 'left', hostPos.left + 'px');
      } else {
        this.renderer.setStyle(this.tooltip, 'top', hostPos.bottom + 'px');
        this.renderer.setStyle(this.tooltip, 'left', hostPos.left + 'px');
      }
    }

    if (this.placement === 'left') {
      this.renderer.setStyle(this.tooltip, 'top', hostPos.top + 'px');
      this.renderer.setStyle(this.tooltip, 'left', hostPos.left - tooltipPos.width + 'px');
    }

    if (this.placement === 'right') {
      this.renderer.setStyle(this.tooltip, 'top', hostPos.top + 'px');
      this.renderer.setStyle(this.tooltip, 'left', hostPos.right + 'px');
    }
  }
}
