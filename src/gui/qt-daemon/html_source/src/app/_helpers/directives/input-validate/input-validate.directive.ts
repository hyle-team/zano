import {Directive, ElementRef, Input, HostListener} from '@angular/core';
import {VariablesService} from '../../services/variables.service';

@Directive({
  selector: '[appInputValidate]'
})
export class InputValidateDirective {

  private type: string;

  constructor(private el: ElementRef, private variablesService: VariablesService) {
  }

  @Input('appInputValidate')
  public set defineInputType(type: string) {
    this.type = type;
  }

  @HostListener('input', ['$event'])
  handleInput(event: Event) {
    if ( this.type === 'money' ) {
      this.moneyValidation(event);
    } else if ( this.type === 'integer' ) {
      this.integerValidation(event);
    }
  }

  private moneyValidation(event: Event) {
    let currentValue = (<HTMLInputElement>event.target).value;
    const originalValue = currentValue;
    const OnlyD = /[^\d\.]/g;
    const _has_error = currentValue.match(OnlyD);
    if (_has_error && _has_error.length) {
      currentValue = currentValue.replace(',', '.').replace(OnlyD, '');
    }
    const _double_separator = currentValue.match(/\./g);
    if (_double_separator && _double_separator.length > 1) {
      currentValue = currentValue.substr(0, currentValue.lastIndexOf('.'));
    }
    if (currentValue.indexOf('.') === 0) {
      currentValue = '0' + currentValue;
    }
    const _zero_fill = currentValue.split('.');
    if (_zero_fill[0].length > 7) {
      _zero_fill[0] = _zero_fill[0].substr(0, 7);
    }

    if (1 in _zero_fill && _zero_fill[1].length) {
      _zero_fill[1] = _zero_fill[1].substr(0, this.variablesService.digits);
    }
    currentValue = _zero_fill.join('.');
    if (currentValue !== originalValue) {
      (<HTMLInputElement>event.target).value = currentValue;
      const cursorPosition = (<HTMLInputElement>event.target).selectionEnd;
      (<HTMLInputElement>event.target).setSelectionRange(cursorPosition, cursorPosition);
      (<HTMLInputElement>event.target).dispatchEvent(new Event('input'));
    }
  }

  private integerValidation(event: Event) {
    let currentValue = (<HTMLInputElement>event.target).value;
    const originalValue = currentValue;
    const OnlyD = /[^\d]/g;
    const _has_error = currentValue.match(OnlyD);
    if (_has_error && _has_error.length) {
      currentValue = currentValue.replace(OnlyD, '');
    }
    if (currentValue !== originalValue) {
      const cursorPosition = (<HTMLInputElement>event.target).selectionEnd;
      (<HTMLInputElement>event.target).value = currentValue;
      (<HTMLInputElement>event.target).setSelectionRange(cursorPosition, cursorPosition);
    }
  }

}




