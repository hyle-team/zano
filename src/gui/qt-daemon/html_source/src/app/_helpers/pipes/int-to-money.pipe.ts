import {Pipe, PipeTransform} from '@angular/core';

@Pipe({
  name: 'intToMoney'
})
export class IntToMoneyPipe implements PipeTransform {

  transform(value: any, args?: any): any {
    if (value === 0 || value === undefined) {
      return '0';
    }
    const power = Math.pow(10, 8);
    let str = (value / power).toFixed(8);
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
