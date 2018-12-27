import {Directive, ElementRef, Input} from '@angular/core';
import * as Inputmask from 'inputmask';

@Directive({
  selector: '[appInputValidate]'
})
export class InputValidateDirective {

  private regexMap = {
    integer: {
      regex: '^[0-9]*$',
      placeholder: ''
    },
    // float: '^[+-]?([0-9]*[.])?[0-9]+$',
    // words: '([A-z]*\\s)*',
    // point25: '^\-?[0-9]*(?:\\.25|\\.50|\\.75|)$',
    money: {
      alias: 'decimal',
      digits: 8,
      max: 999999999999,
      rightAlign: false,
      allowMinus: false,
      onBeforeMask: (value) => value
    }
  };

  constructor(private el: ElementRef) {
  }

  @Input('appInputValidate')
  public set defineInputType(type: string) {
    Inputmask(this.regexMap[type]).mask(this.el.nativeElement);
  }

}




