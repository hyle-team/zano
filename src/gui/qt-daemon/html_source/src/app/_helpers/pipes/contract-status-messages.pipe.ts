import {Pipe, PipeTransform} from '@angular/core';
import {TranslateService} from '@ngx-translate/core';

@Pipe({
  name: 'contractStatusMessages'
})
export class ContractStatusMessagesPipe implements PipeTransform {

  constructor(private translate: TranslateService) {}

  getStateSeller(stateNum: number): string {
    const state = {part1: '', part2: ''};
    switch (stateNum) {
      case 1:
        state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.SELLER.NEW_CONTRACT');
        break;
      case 110:
        state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.SELLER.IGNORED');
        break;
      case 201:
        state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.SELLER.ACCEPTED');
        state.part2 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.SELLER.WAIT');
        break;
      case 2:
        state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.SELLER.BUYER_WAIT');
        state.part2 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.SELLER.PLEDGES_MADE');
        break;
      case 3:
        state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.SELLER.COMPLETED');
        state.part2 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.SELLER.RECEIVED');
        break;
      case 4:
        state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.SELLER.NOT_RECEIVED');
        state.part2 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.SELLER.NULLIFIED');
        break;
      case 5:
        state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.SELLER.PROPOSAL_CANCEL');
        break;
      case 601:
        state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.SELLER.BEING_CANCELLED');
        break;
      case 6:
        state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.SELLER.CANCELLED');
        state.part2 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.SELLER.PLEDGES_RETURNED');
        break;
      case 130:
        state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.SELLER.IGNORED_CANCEL');
        break;
      case 140:
        state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.SELLER.EXPIRED');
        break;
    }
    return state.part1 + ' ' + state.part2;
  }

  getStateBuyer(stateNum: number): string {
    const state = {part1: '', part2: ''};
    switch (stateNum) {
      case 1:
        state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.WAITING');
        state.part2 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.PLEDGE_RESERVED');
        break;
      case 110:
        state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.IGNORED');
        state.part2 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.PLEDGE_UNBLOCKED');
        break;
      case 201:
        state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.ACCEPTED');
        state.part2 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.WAIT');
        break;
      case 2:
        state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.ACCEPTED');
        state.part2 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.PLEDGES_MADE');
        break;
      case 120:
        state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.WAITING_SELLER');
        state.part2 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.PLEDGES_MADE');
        break;
      case 3:
        state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.COMPLETED');
        state.part2 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.RECEIVED');
        break;
      case 4:
        state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.NOT_RECEIVED');
        state.part2 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.NULLIFIED');
        break;
      case 5:
        state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.WAITING_CANCEL');
        break;
      case 601:
        state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.BEING_CANCELLED');
        break;
      case 6:
        state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.CANCELLED');
        state.part2 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.PLEDGES_RETURNED');
        break;
      case 130:
        state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.IGNORED_CANCEL');
        break;
      case 140:
        state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.EXPIRED');
        break;
    }
    return state.part1 + ' ' + state.part2;
  }

  transform(item: any, args?: any): any {
    if (item.is_a) {
      return this.getStateBuyer(item.state);
    } else {
      return this.getStateSeller(item.state);
    }
  }

}
