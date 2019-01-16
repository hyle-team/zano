import {Injectable, Input, NgZone} from '@angular/core';
import {Wallet} from '../models/wallet.model';
import {BehaviorSubject} from 'rxjs';
import {Idle} from 'idlejs/dist';
import {Router} from '@angular/router';
import {ContextMenuComponent, ContextMenuService} from 'ngx-contextmenu';

@Injectable({
  providedIn: 'root'
})
export class VariablesService {

  public digits = 12;
  public appPass = '';
  public moneyEquivalent = 0;
  public defaultTheme = 'dark';
  public defaultCurrency = 'ZAN';
  public opening_wallet: Wallet;
  public exp_med_ts = 0;
  public height_app = 0;
  public daemon_state = 0;
  public sync = {
    progress_value: 0,
    progress_value_text: '0'
  };

  public settings = {
    theme: '',
    language: 'en',
    default_path: '/',
    viewedContracts: [],
    notViewedContracts: []
  };

  public wallets: Array<Wallet> = [];
  public currentWallet: Wallet;

  getHeightAppEvent = new BehaviorSubject(null);
  getRefreshStackingEvent = new BehaviorSubject(null);

  public idle = new Idle()
    .whenNotInteractive()
    .within(15)
    .do(() => {
      this.ngZone.run(() => {
        this.idle.stop();
        this.appPass = '';
        this.router.navigate(['/login'], {queryParams: {type: 'auth'}});
      });
    });

  @Input() contextMenu: ContextMenuComponent;

  constructor(private router: Router, private ngZone: NgZone, private contextMenuService: ContextMenuService) {
  }

  setHeightApp(height: number) {
    if (height !== this.height_app) {
      this.height_app = height;
      this.getHeightAppEvent.next(height);
    }
  }

  setRefreshStacking(wallet_id: number) {
    this.getHeightAppEvent.next(wallet_id);
  }

  setCurrentWallet(id): void {
    this.wallets.forEach((wallet) => {
      if (wallet.wallet_id === id) {
        this.currentWallet = wallet;
      }
    });
  }

  getWallet(id): Wallet {
    for (let i = 0; i < this.wallets.length; i++) {
      if (this.wallets[i].wallet_id === id) {
        return this.wallets[i];
      }
    }
    return null;
  }

  startCountdown() {
    this.idle.start();
  }

  stopCountdown() {
    this.idle.stop();
  }

  public onContextMenu($event: MouseEvent): void {
    $event.target['contextSelectionStart'] = $event.target['selectionStart'];
    $event.target['contextSelectionEnd'] = $event.target['selectionEnd'];
    if ($event.target && ($event.target['nodeName'].toUpperCase() === 'TEXTAREA' || $event.target['nodeName'].toUpperCase() === 'INPUT') && !$event.target['readOnly']) {
      this.contextMenuService.show.next({
        contextMenu: this.contextMenu,
        event: $event,
        item: $event.target,
      });
      $event.preventDefault();
      $event.stopPropagation();
    }
  }

}
