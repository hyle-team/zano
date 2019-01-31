import {BigNumber} from 'bignumber.js';

export class Transaction {
  amount: BigNumber;
  comment: string;
  contract: any[];
  fee: BigNumber;
  height: number;
  is_income: boolean;
  is_mining: boolean;
  is_mixing: boolean;
  is_service: boolean;
  payment_id: string;
  show_sender: boolean;
  td: object;
  timestamp: number;
  tx_blob_size: number;
  tx_hash: string;
  tx_type: number;
  unlock_time: number;

  sortAmount?: BigNumber;
  sortFee?: BigNumber;
}
