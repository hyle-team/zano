import { pluck, distinctUntilChanged } from 'rxjs/operators';
import { Observable, BehaviorSubject } from 'rxjs';
import { Wallet } from './app/_helpers/models/wallet.model';

export interface State {
  wallets: Wallet[];
  [key: string]: any;
}

const state: State = {
  wallets: undefined,
};

export class Store {
  private subject = new BehaviorSubject<State>(state);
  private store = this.subject.asObservable().pipe(distinctUntilChanged());

  get value() {
    return this.subject.value;
  }

  select<T>(name: string): Observable<T> {
    return this.store.pipe(pluck(name));
  }

  // tslint:disable-next-line:no-shadowed-variable
  set(name: string, state: any) {
    this.subject.next({ ...this.value, [name]: state });
  }
}
