import {Directive, HostListener} from '@angular/core';

@Directive({
  selector: 'input'
})
export class InputDisableSelectionDirective {

  constructor() {}

  @HostListener('mousedown', ['$event'])
  handleInput(event: Event) {
    if ((<HTMLInputElement>event.target).readOnly) {
      event.preventDefault();
    }
  }
}
