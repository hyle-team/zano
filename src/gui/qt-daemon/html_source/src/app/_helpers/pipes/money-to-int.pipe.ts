import {Pipe, PipeTransform} from '@angular/core';
import {VariablesService} from '../services/variables.service';
import {BigNumber} from 'bignumber.js';

@Pipe({
  name: 'moneyToInt'
})
export class MoneyToIntPipe implements PipeTransform {

  constructor(private variablesService: VariablesService) {}

  transform(value: any, args?: any): any {
    const CURRENCY_DISPLAY_DECIMAL_POINT = this.variablesService.digits;
    let result;
    if (value) {
      let am_str = value.toString().trim();
      const point_index = am_str.indexOf('.');
      let fraction_size = 0;
      if (-1 !== point_index) {
        fraction_size = am_str.length - point_index - 1;
        while (CURRENCY_DISPLAY_DECIMAL_POINT < fraction_size && '0' === am_str[am_str.length - 1]) {
          am_str = am_str.slice(0, am_str.length - 1);
          --fraction_size;
        }
        if (CURRENCY_DISPLAY_DECIMAL_POINT < fraction_size) {
          return undefined;
        }
        am_str = am_str.slice(0, point_index) + am_str.slice(point_index + 1, am_str.length);
      } else {
        fraction_size = 0;
      }
      if (!am_str.length) {
        return undefined;
      }
      if (fraction_size < CURRENCY_DISPLAY_DECIMAL_POINT) {
        for (let i = 0; i !== CURRENCY_DISPLAY_DECIMAL_POINT - fraction_size; i++) {
          am_str = am_str + '0';
        }
      }
      result = (new BigNumber(am_str)).integerValue();
    }
    return result;
  }

}
