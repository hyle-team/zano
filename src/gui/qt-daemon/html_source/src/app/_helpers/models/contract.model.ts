export class Contract {
  contract_id: string;
  is_a: boolean;
  cancel_expiration_time: number;
  expiration_time: number;
  height: number;
  timestamp: number;

  state: number;
  private_detailes: any;
  is_new: boolean;


  constructor(contract_id, is_a, cancel_expiration_time, expiration_time, height, timestamp) {
    this.contract_id = contract_id;
    this.is_a = is_a;
    this.cancel_expiration_time = cancel_expiration_time;
    this.expiration_time = expiration_time;
    this.height = height;
    this.timestamp = timestamp;
  }


}
