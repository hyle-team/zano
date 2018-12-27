import {Pipe, PipeTransform} from '@angular/core';
import {VariablesService} from '../services/variables.service';
import {TranslateService} from '@ngx-translate/core';

@Pipe({
  name: 'contractTimeLeft'
})
export class ContractTimeLeftPipe implements PipeTransform {

  constructor(private service: VariablesService, private translate: TranslateService) {}

  transform(value: any, arg?: any): any {
    const time = parseInt(((parseInt(value, 10) - this.service.exp_med_ts) / 3600).toFixed(0), 10);
    const type = arg || 0;
    if (time === 0) {
      return this.translate.instant('CONTRACTS.TIME_LEFT.REMAINING_LESS_ONE');
    }
    if (this.service.settings.language === 'en') {
      if (type === 0) {
        if (time === 1) {
          return this.translate.instant('CONTRACTS.TIME_LEFT.REMAINING_ONE', {time: time});
        } else {
          return this.translate.instant('CONTRACTS.TIME_LEFT.REMAINING_MANY', {time: time});
        }
      } else if (type === 1) {
        if (time === 1) {
          return this.translate.instant('CONTRACTS.TIME_LEFT.REMAINING_ONE_RESPONSE', {time: time});
        } else {
          return this.translate.instant('CONTRACTS.TIME_LEFT.REMAINING_MANY_RESPONSE', {time: time});
        }
      } else if (type === 2) {
        if (time === 1) {
          return this.translate.instant('CONTRACTS.TIME_LEFT.REMAINING_ONE_WAITING', {time: time});
        } else {
          return this.translate.instant('CONTRACTS.TIME_LEFT.REMAINING_MANY_WAITING', {time: time});
        }
      }
    } else {
      const rest = time % 10;
      if (type === 0) {
        if (((time > 20 ) && (rest === 1)) || time === 1) {
          return this.translate.instant('CONTRACTS.TIME_LEFT.REMAINING_ONE', {time: time});
        } else if ((time > 1) && (time < 5) || ((time > 20 ) && (rest === 2 || rest === 3 || rest === 4))) {
          return this.translate.instant('CONTRACTS.TIME_LEFT.REMAINING_MANY', {time: time});
        } else {
          return this.translate.instant('CONTRACTS.TIME_LEFT.REMAINING_MANY_ALT', {time: time});
        }
      } else if (type === 1) {
        if (((time > 20 ) && (rest === 1)) || time === 1) {
          return this.translate.instant('CONTRACTS.TIME_LEFT.REMAINING_ONE_RESPONSE', {time: time});
        } else if ((time > 1) && (time < 5) || ((time > 20 ) && (rest === 2 || rest === 3 || rest === 4))) {
          return this.translate.instant('CONTRACTS.TIME_LEFT.REMAINING_MANY_RESPONSE', {time: time});
        } else {
          return this.translate.instant('CONTRACTS.TIME_LEFT.REMAINING_MANY_ALT_RESPONSE', {time: time});
        }
      } else if (type === 2) {
        if (((time > 20 ) && (rest === 1)) || time === 1) {
          return this.translate.instant('CONTRACTS.TIME_LEFT.REMAINING_ONE_WAITING', {time: time});
        } else if ((time > 1) && (time < 5) || ((time > 20 ) && (rest === 2 || rest === 3 || rest === 4))) {
          return this.translate.instant('CONTRACTS.TIME_LEFT.REMAINING_MANY_WAITING', {time: time});
        } else {
          return this.translate.instant('CONTRACTS.TIME_LEFT.REMAINING_MANY_ALT_WAITING', {time: time});
        }
      }
    }
    return null;
  }

}
