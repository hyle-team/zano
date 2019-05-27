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
        state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.SELLER.WAITING_BUYER');
        break;
      case 3:
        state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.SELLER.COMPLETED');
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
        break;
      case 130:
        state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.SELLER.IGNORED_CANCEL');
        break;
      case 140:
        state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.SELLER.EXPIRED');
        break;
    }
    return state.part1 + (state.part2.length ? '. ' + state.part2 : '');
  }

  getStateBuyer(stateNum: number): string {
    const state = {part1: '', part2: ''};
    switch (stateNum) {
      case 1:
        state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.WAITING');
        break;
      case 110:
        state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.IGNORED');
        break;
      case 201:
        state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.ACCEPTED');
        state.part2 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.WAIT');
        break;
      case 2:
        state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.ACCEPTED');
        break;
      case 120:
        state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.WAITING_SELLER');
        break;
      case 3:
        state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.COMPLETED');
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
        break;
      case 130:
        state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.IGNORED_CANCEL');
        break;
      case 140:
        state.part1 = this.translate.instant('CONTRACTS.STATUS_MESSAGES.BUYER.EXPIRED');
        break;
    }
    return state.part1 + (state.part2.length ? '. ' + state.part2 : '');
  }

  transform(state: number, is_a?: boolean): any {
    if (is_a) {
      return this.getStateBuyer(state);
    } else {
      return this.getStateSeller(state);
    }
  }

}
