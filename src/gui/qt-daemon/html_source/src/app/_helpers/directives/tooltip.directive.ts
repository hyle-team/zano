import {Directive, Input, ElementRef, HostListener, Renderer2, HostBinding, OnDestroy} from '@angular/core';
import {ActivatedRoute} from '@angular/router';

@Directive({
  selector: '[tooltip]'
})

export class TooltipDirective implements OnDestroy {

  @HostBinding('style.cursor') cursor = 'pointer';

  @Input('tooltip') tooltipInner: any;
  @Input() placement: string;
  @Input() tooltipClass: string;
  @Input() timeout = 0;
  @Input() delay = 0;
  tooltip: HTMLElement;

  removeTooltipTimeout;
  removeTooltipTimeoutInner;

  constructor(private el: ElementRef, private renderer: Renderer2, private route: ActivatedRoute) {
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
    this.removeTooltipTimeout = setTimeout(() => {
      this.renderer.setStyle(this.tooltip, 'opacity', '0');
      this.removeTooltipTimeoutInner = setTimeout(() => {
        this.renderer.removeChild(document.body, this.tooltip);
        this.tooltip = null;
      }, this.delay);
    }, this.timeout);
  }

  cancelHide() {
    clearTimeout(this.removeTooltipTimeout);
    clearTimeout(this.removeTooltipTimeoutInner);
    this.renderer.setStyle(this.tooltip, 'opacity', '1');
  }

  create() {
    if (typeof this.tooltipInner === 'string') {
      this.tooltip = this.renderer.createElement('div');
      this.tooltip.innerHTML = this.tooltipInner;
    } else {
      this.tooltip = this.tooltipInner;
    }
    this.renderer.appendChild(document.body, this.tooltip);

    this.tooltip.addEventListener('mouseenter', () => {
      this.cancelHide();
    });
    this.tooltip.addEventListener('mouseleave', () => {
      if (this.tooltip) {
        this.hide();
      }
    });

    this.renderer.setStyle(document.body, 'position', 'relative');
    this.renderer.setStyle(this.tooltip, 'position', 'absolute');
    if (this.tooltipClass !== null) {
      const classes = this.tooltipClass.split(' ');
      for (let i = 0; i < classes.length; i++) {
        this.renderer.addClass(this.tooltip, classes[i]);
      }
    }
    if (this.placement !== null) {
      this.renderer.addClass(this.tooltip, 'ng-tooltip-' + this.placement);
    } else {
      this.placement = 'top';
      this.renderer.addClass(this.tooltip, 'ng-tooltip-top');
    }
    this.renderer.setStyle(this.tooltip, 'opacity', '0');
    this.renderer.setStyle(this.tooltip, '-webkit-transition', `opacity ${this.delay}ms`);
    this.renderer.setStyle(this.tooltip, '-moz-transition', `opacity ${this.delay}ms`);
    this.renderer.setStyle(this.tooltip, '-o-transition', `opacity ${this.delay}ms`);
    this.renderer.setStyle(this.tooltip, 'transition', `opacity ${this.delay}ms`);
    window.setTimeout(() => {
      this.renderer.setStyle(this.tooltip, 'opacity', '1');
    }, 0);
  }

  setPosition() {
    const hostPos = this.el.nativeElement.getBoundingClientRect();
    // const scrollPos = window.pageYOffset || document.documentElement.scrollTop || document.body.scrollTop || 0;

    if (this.placement === 'top') {
      this.renderer.setStyle(this.tooltip, 'left', hostPos.left + 'px');
      this.renderer.setStyle(this.tooltip, 'top', hostPos.top - this.tooltip.getBoundingClientRect().height + 'px');
    }

    if (this.placement === 'bottom') {
      if (window.innerHeight < hostPos.bottom + this.tooltip.offsetHeight + parseInt(getComputedStyle(this.tooltip).marginTop, 10)) {
        this.renderer.removeClass(this.tooltip, 'ng-tooltip-bottom');
        this.renderer.addClass(this.tooltip, 'ng-tooltip-top');
        this.renderer.setStyle(this.tooltip, 'left', hostPos.left + 'px');
        this.renderer.setStyle(this.tooltip, 'top', hostPos.top - this.tooltip.getBoundingClientRect().height + 'px');
      } else {
        this.renderer.setStyle(this.tooltip, 'top', hostPos.bottom + 'px');
        this.renderer.setStyle(this.tooltip, 'left', hostPos.left + 'px');
      }
    }

    if (this.placement === 'left') {
      this.renderer.setStyle(this.tooltip, 'top', hostPos.top + 'px');
      this.renderer.setStyle(this.tooltip, 'left', hostPos.left - this.tooltip.getBoundingClientRect().width + 'px');
    }

    if (this.placement === 'right') {
      this.renderer.setStyle(this.tooltip, 'top', hostPos.top + 'px');
      this.renderer.setStyle(this.tooltip, 'left', hostPos.right + 'px');
    }
  }

  ngOnDestroy() {
    clearTimeout(this.removeTooltipTimeout);
    clearTimeout(this.removeTooltipTimeoutInner);
    if (this.tooltip) {
      this.renderer.removeChild(document.body, this.tooltip);
      this.tooltip = null;
    }
  }

}
