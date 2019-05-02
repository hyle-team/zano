import {Pipe, PipeTransform} from '@angular/core';
import {VariablesService} from '../services/variables.service';
import {BigNumber} from 'bignumber.js';

@Pipe({
  name: 'intToMoney'
})
export class IntToMoneyPipe implements PipeTransform {

  constructor(private variablesService: VariablesService) {}

  transform(value: any, args?: any): any {
    if (value === 0 || value === undefined) {
      return '0';
    }
    let maxFraction = this.variablesService.digits;
    if (args) {
      maxFraction = parseInt(args, 10);
    }
    const power = Math.pow(10, this.variablesService.digits);
    let str = (new BigNumber(value)).div(power).toFixed(maxFraction);

    for (let i = str.length - 1; i >= 0; i--) {
      if (str[i] !== '0') {
        str = str.substr(0, i + 1);
        break;
      }
    }
    if (str[str.length - 1] === '.') {
      str = str.substr(0, str.length - 1);
    }
    return str;
  }

}
