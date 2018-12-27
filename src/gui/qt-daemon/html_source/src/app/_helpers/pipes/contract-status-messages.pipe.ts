import {Pipe, PipeTransform} from '@angular/core';

@Pipe({
  name: 'contractStatusMessages'
})
export class ContractStatusMessagesPipe implements PipeTransform {

  getStateSeller(stateNum: number): string {
    const state = {part1: '', part2: ''};
    switch (stateNum) {
      case 1:
        state.part1 = 'New contract proposal';
        break;
      case 110:
        state.part1 = 'You ignored the contract proposal';
        break;
      case 201:
        state.part1 = 'You have accepted the contract proposal';
        state.part2 = 'Please wait for the pledges to be made';
        break;
      case 2:
        state.part1 = 'The buyer is waiting for the item to be delivered';
        state.part2 = 'Pledges made';
        break;
      case 3:
        state.part1 = 'Contract completed successfully';
        state.part2 = 'Item received, payment transferred, pledges returned';
        break;
      case 4:
        state.part1 = 'Item not received';
        state.part2 = 'All pledges nullified';
        break;
      case 5:
        state.part1 = 'New proposal to cancel contract and return pledges';
        break;
      case 601:
        state.part1 = 'The contract is being cancelled. Please wait for the pledge to be returned';
        break;
      case 6:
        state.part1 = 'Contract canceled';
        state.part2 = 'Pledges returned';
        break;
      case 130:
        state.part1 = 'You ignored the proposal to cancel the contract';
        break;
      case 140:
        state.part1 = 'The contract proposal has expired';
        break;
    }
    return state.part1 + ' ' + state.part2;
  }

  getStateCustomer(stateNum: number): string {
    const state = {part1: '', part2: ''};
    switch (stateNum) {
      case 1:
        state.part1 = 'Waiting for seller respond to contract proposal';
        state.part2 = 'Pledge amount reserved';
        break;
      case 110:
        state.part1 = 'The seller ignored your contract proposal';
        state.part2 = 'Pledge amount unblocked';
        break;
      case 201:
        state.part1 = 'The seller accepted your contract proposal';
        state.part2 = 'Please wait for the pledges to be made';
        break;
      case 2:
        state.part1 = 'The seller accepted your contract proposal';
        state.part2 = 'Pledges made';
        break;
      case 120:
        state.part1 = 'Waiting for seller to ship item';
        state.part2 = 'Pledges made';
        break;
      case 3:
        state.part1 = 'Contract completed successfully';
        state.part2 = 'Item received, payment transferred, pledges returned';
        break;
      case 4:
        state.part1 = 'Item not received';
        state.part2 = 'All pledges nullified';
        break;
      case 5:
        state.part1 = 'Waiting for seller to respond to proposal to cancel contract and return pledges';
        break;
      case 601:
        state.part1 = 'The contract is being cancelled. Please wait for the pledge to be returned';
        break;
      case 6:
        state.part1 = 'Contract canceled';
        state.part2 = 'Pledges returned';
        break;
      case 130:
        state.part1 = 'The seller ignored your proposal to cancel the contract';
        break;
      case 140:
        state.part1 = 'The contract proposal has expired';
        break;
    }
    return state.part1 + ' ' + state.part2;
  }

  transform(item: any, args?: any): any {

    if (item.is_a) {
      return this.getStateCustomer(item.state);
    } else {
      return this.getStateSeller(item.state);
    }
  }

}
