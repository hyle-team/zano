import {Pipe, PipeTransform} from '@angular/core';
import {TranslateService} from '@ngx-translate/core';

@Pipe({
  name: 'historyTypeMessages'
})
export class HistoryTypeMessagesPipe implements PipeTransform {

  constructor(private translate: TranslateService) {}

  transform(item: any, args?: any): any {

    if (item.tx_type === 0) {
      if (item.remote_addresses && item.remote_addresses[0]) {
        return item.remote_addresses[0];
      } else {
        if (item.is_income) {
          return this.translate.instant('HISTORY.TYPE_MESSAGES.HIDDEN');
        } else {
          return this.translate.instant('HISTORY.TYPE_MESSAGES.UNDEFINED');
        }
      }
    } else if (item.tx_type === 6 && item.height === 0) {
      return 'unknown';
    } else if (item.tx_type === 9) {
      if (item.hasOwnProperty('contract') && item.contract[0].is_a) {
        return this.translate.instant('HISTORY.TYPE_MESSAGES.COMPLETE_BUYER');
      } else {
        return this.translate.instant('HISTORY.TYPE_MESSAGES.COMPLETE_SELLER');
      }
    } else {
      switch (item.tx_type) {
        // case 0:
        //   return '';
        // case 1:
        //   return '';
        // case 2:
        //   return '';
        // case 3:
        //   return '';
        case 4:
          return this.translate.instant('HISTORY.TYPE_MESSAGES.CREATE_ALIAS');
        case 5:
          return this.translate.instant('HISTORY.TYPE_MESSAGES.UPDATE_ALIAS');
        case 6:
          return (item.td['spn'] && item.td['spn'].length) ? this.translate.instant('HISTORY.TYPE_MESSAGES.POS_REWARD') : this.translate.instant('HISTORY.TYPE_MESSAGES.POW_REWARD');
        case 7:
          return this.translate.instant('HISTORY.TYPE_MESSAGES.CREATE_CONTRACT');
        case 8:
          return this.translate.instant('HISTORY.TYPE_MESSAGES.PLEDGE_CONTRACT');
        // case 9:
        //   return '';
        case 10:
          return this.translate.instant('HISTORY.TYPE_MESSAGES.NULLIFY_CONTRACT');
        case 11:
          return this.translate.instant('HISTORY.TYPE_MESSAGES.PROPOSAL_CANCEL_CONTRACT');
        case 12:
          return this.translate.instant('HISTORY.TYPE_MESSAGES.CANCEL_CONTRACT');
      }
    }

    return this.translate.instant('HISTORY.TYPE_MESSAGES.UNDEFINED');
  }

}
