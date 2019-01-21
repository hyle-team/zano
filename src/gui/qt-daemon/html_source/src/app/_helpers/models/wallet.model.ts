import {Contract} from './contract.model';
import {Transaction} from './transaction.model';
import {BigNumber} from 'bignumber.js';

export class Wallet {
  wallet_id: number;
  name: string;
  pass: string;
  path: string;
  address: string;
  balance: BigNumber;
  unlocked_balance: BigNumber;
  mined_total: number;
  tracking_hey: string;

  alias?: string;
  staking?: boolean;
  new_messages?: number;
  new_contracts?: number;

  history: Array<Transaction> = [];
  excluded_history: Array<Transaction> = [];

  contracts: Array<Contract> = [];

  progress?: number;
  loaded?: boolean;

  constructor(id, name, pass, path, address, balance, unlocked_balance, mined = 0, tracking = '') {
    this.wallet_id = id;
    this.name = name;
    this.pass = pass;
    this.path = path;
    this.address = address;
    this.balance = balance;
    this.unlocked_balance = unlocked_balance;
    this.mined_total = mined;
    this.tracking_hey = tracking;

    this.alias = '';
    this.staking = false;
    this.new_messages = 0;
    this.new_contracts = 0;

    this.history = [];
    this.excluded_history = [];

    this.progress = 0;
    this.loaded = false;
  }

  getMoneyEquivalent(equivalent) {
    return this.balance.multipliedBy(equivalent).toFixed(0);
  }

  havePass(): boolean {
    return (this.pass !== '' && this.pass !== null);
  }

  isActive(id): boolean {
    return this.wallet_id === id;
  }

  prepareHistoryItem(item: Transaction): any {
    if (item.tx_type === 4) {
      item.sortFee = item.amount.plus(item.fee).negated();
      item.sortAmount = new BigNumber(0);
    } else if (item.tx_type === 3) {
      item.sortFee = new BigNumber(0);
    } else if ((item.hasOwnProperty('contract') && (item.contract[0].state === 3 || item.contract[0].state === 6 || item.contract[0].state === 601) && !item.contract[0].is_a)) {
      item.sortFee = item.fee.negated();
      item.sortAmount = item.amount.negated();
    } else {
      if (!item.is_income) {
        item.sortFee = item.fee.negated();
        item.sortAmount = item.amount.negated();
      } else {
        item.sortAmount = item.amount;
      }
    }
    return item;
  }

  prepareHistory(items: Transaction[]): void {
    for (let i = 0; i < items.length; i++) {
      if ((items[i].tx_type === 7 && items[i].is_income) || (items[i].tx_type === 11 && items[i].is_income) || (items[i].amount.eq(0) && items[i].fee.eq(0))) {
        let exists = false;
        for (let j = 0; j < this.excluded_history.length; j++) {
          if (this.excluded_history[j].tx_hash === items[i].tx_hash) {
            exists = true;
            if (this.excluded_history[j].height !== items[i].height) {
              this.excluded_history[j] = items[i];
            }
            break;
          }
        }
        if (!exists) {
          this.excluded_history.push(items[i]);
        }
      } else {
        let exists = false;
        for (let j = 0; j < this.history.length; j++) {
          if (this.history[j].tx_hash === items[i].tx_hash) {
            exists = true;
            if (this.history[j].height !== items[i].height) {
              this.history[j] = this.prepareHistoryItem(items[i]);
            }
            break;
          }
        }
        if (!exists) {
          if (this.history.length && items[i].timestamp > this.history[0].timestamp) {
            this.history.unshift(this.prepareHistoryItem(items[i]));
          } else {
            this.history.push(this.prepareHistoryItem(items[i]));
          }
        }
      }
    }
  }

  prepareContractsAfterOpen(items: any[], exp_med_ts, height_app, viewedContracts, notViewedContracts): void {
    const safe = this;
    for (let i = 0; i < items.length; i++) {
      const contract = items[i];
      let contractTransactionExist = false;
      if (safe && safe.history) {
        contractTransactionExist = safe.history.some(elem => elem.contract && elem.contract.length && elem.contract[0].contract_id === contract.contract_id);
      }
      if (!contractTransactionExist && safe && safe.excluded_history) {
        contractTransactionExist = safe.excluded_history.some(elem => elem.contract && elem.contract.length && elem.contract[0].contract_id === contract.contract_id);
      }

      if (!contractTransactionExist) {
        contract.state = 140;
      } else if (contract.state === 1 && contract.expiration_time < exp_med_ts) {
        contract.state = 110;
      } else if (contract.state === 2 && contract.cancel_expiration_time !== 0 && contract.cancel_expiration_time < exp_med_ts && contract.height === 0) {
        const searchResult1 = viewedContracts.some(elem => elem.state === 2 && elem.is_a === contract.is_a && elem.contract_id === contract.contract_id);
        if (!searchResult1) {
          contract.state = 130;
          contract.is_new = true;
        }
      } else if (contract.state === 1) {
        const searchResult2 = notViewedContracts.find(elem => elem.state === 110 && elem.is_a === contract.is_a && elem.contract_id === contract.contract_id);
        if (searchResult2) {
          if (searchResult2.time === contract.expiration_time) {
            contract.state = 110;
          } else {
            for (let j = 0; j < notViewedContracts.length; j++) {
              if (notViewedContracts[j].contract_id === contract.contract_id && notViewedContracts[j].is_a === contract.is_a) {
                notViewedContracts.splice(j, 1);
                break;
              }
            }
            for (let j = 0; j < viewedContracts.length; j++) {
              if (viewedContracts[j].contract_id === contract.contract_id && viewedContracts[j].is_a === contract.is_a) {
                viewedContracts.splice(j, 1);
                break;
              }
            }
          }
        }
      } else if (contract.state === 2 && (contract.height === 0 || (height_app - contract.height) < 10)) {
        contract.state = 201;
      } else if (contract.state === 2) {
        const searchResult3 = viewedContracts.some(elem => elem.state === 120 && elem.is_a === contract.is_a && elem.contract_id === contract.contract_id);
        if (searchResult3) {
          contract.state = 120;
        }
      } else if (contract.state === 5) {
        const searchResult4 = notViewedContracts.find(elem => elem.state === 130 && elem.is_a === contract.is_a && elem.contract_id === contract.contract_id);
        if (searchResult4) {
          if (searchResult4.time === contract.cancel_expiration_time) {
            contract.state = 130;
          } else {
            for (let j = 0; j < notViewedContracts.length; j++) {
              if (notViewedContracts[j].contract_id === contract.contract_id && notViewedContracts[j].is_a === contract.is_a) {
                notViewedContracts.splice(j, 1);
                break;
              }
            }
            for (let j = 0; j < viewedContracts.length; j++) {
              if (viewedContracts[j].contract_id === contract.contract_id && viewedContracts[j].is_a === contract.is_a) {
                viewedContracts.splice(j, 1);
                break;
              }
            }
          }
        }
      } else if (contract.state === 6 && (contract.height === 0 || (height_app - contract.height) < 10)) {
        contract.state = 601;
      }
      const searchResult = viewedContracts.some(elem => elem.state === contract.state && elem.is_a === contract.is_a && elem.contract_id === contract.contract_id);
      contract.is_new = !searchResult;

      contract['private_detailes'].a_pledge = contract['private_detailes'].a_pledge.plus(contract['private_detailes'].to_pay);

      safe.contracts.push(contract);
    }
    this.recountNewContracts();
  }

  recountNewContracts() {
    this.new_contracts = (this.contracts.filter(item => item.is_new === true )).length;
  }

  getContract(id): Contract {
    for (let i = 0; i < this.contracts.length; i++) {
      if (this.contracts[i].contract_id === id) {
        return this.contracts[i];
      }
    }
    return null;
  }

}
