import {Directive, ElementRef, Input} from '@angular/core';
import * as Inputmask from 'inputmask';
import {VariablesService} from '../../services/variables.service';

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
      digits: this.variablesService.digits,
      max: 99999999.999999999999,
      rightAlign: false,
      allowMinus: false,
      onBeforeMask: (value) => value
    }
  };

  constructor(private el: ElementRef, private variablesService: VariablesService) {
  }

  @Input('appInputValidate')
  public set defineInputType(type: string) {
    Inputmask(this.regexMap[type]).mask(this.el.nativeElement);
  }

}




