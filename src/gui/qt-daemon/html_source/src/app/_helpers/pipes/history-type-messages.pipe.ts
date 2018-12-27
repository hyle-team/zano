import {Pipe, PipeTransform} from '@angular/core';

@Pipe({
  name: 'historyTypeMessages'
})
export class HistoryTypeMessagesPipe implements PipeTransform {

  transform(item: any, args?: any): any {

    if (item.tx_type === 0) {
      if (item.remote_addresses && item.remote_addresses[0]) {
        return item.remote_addresses[0];
      } else {
        if (item.is_income) {
          return 'hidden';
        } else {
          return 'Undefined';
        }
      }
    } else if (item.tx_type === 6 && item.height === 0) {
      return 'unknown';
    } else if (item.tx_type === 9) {
      if (item.hasOwnProperty('contract') && item.contract[0].is_a) {
        return 'Successfully complete contract, return remaining pledge';
      } else {
        return 'Successfully complete contract, receive payment on contract, and return pledge';
      }
    } else {
      switch (item.tx_type) {
        // case 0:
        //   return $filter('translate')('GUI_TX_TYPE_NORMAL');
        //   break;
        // case 1:
        //   return $filter('translate')('GUI_TX_TYPE_PUSH_OFFER');
        // case 2:
        //   return $filter('translate')('GUI_TX_TYPE_UPDATE_OFFER');
        // case 3:
        //   return $filter('translate')('GUI_TX_TYPE_CANCEL_OFFER');
        // case 4:
        //   return $filter('translate')('GUI_TX_TYPE_NEW_ALIAS');
        // case 5:
        //   return $filter('translate')('GUI_TX_TYPE_UPDATE_ALIAS');
        case 6:
          return 'Mined funds';
        case 7:
          return 'Send contract offer';
        case 8:
          return 'Make pledge on offer';
        // case 9:
        //    return $filter('translate')('GUI_TX_TYPE_ESCROW_RELEASE_NORMAL');
        //    break;
        case 10:
          return 'Nullify pledges for contract';
        case 11:
          return 'Send proposal to cancel contract';
        case 12:
          return 'Cancel contract, return pledges';
      }
    }

    return 'Undefined';
  }

}
