import {Injectable, NgZone} from '@angular/core';
import {Wallet} from '../models/wallet.model';
import {BehaviorSubject} from 'rxjs';
import {Idle} from 'idlejs/dist';
import {Router} from '@angular/router';
import {ContextMenuComponent, ContextMenuService} from 'ngx-contextmenu';
import {BigNumber} from 'bignumber.js';

@Injectable({
  providedIn: 'root'
})
export class VariablesService {

  public digits = 12;
  public appPass = '';
  public appLogin = false;
  public moneyEquivalent = 0;
  public defaultTheme = 'dark';
  public defaultCurrency = 'ZAN';
  public opening_wallet: Wallet;
  public exp_med_ts = 0;
  public height_app = 0;
  public last_build_available = '';
  public daemon_state = 3;
  public sync = {
    progress_value: 0,
    progress_value_text: '0'
  };
  public default_fee = '0.010000000000';
  public default_fee_big = new BigNumber('10000000000');

  public settings = {
    appLockTime: 15,
    appLog: 0,
    theme: '',
    scale: 10,
    language: 'en',
    default_path: '/',
    viewedContracts: [],
    notViewedContracts: [],
    wallets: []
  };

  public wallets: Array<Wallet> = [];
  public currentWallet: Wallet;
  public aliases: any = [];
  public aliasesChecked: any = {};
  public enableAliasSearch = false;
  public maxWalletNameLength = 25;
  public maxCommentLength = 255;

  getExpMedTsEvent = new BehaviorSubject(null);
  getHeightAppEvent = new BehaviorSubject(null);
  getRefreshStackingEvent = new BehaviorSubject(null);
  getAliasChangedEvent = new BehaviorSubject(null);

  public idle = new Idle()
    .whenNotInteractive()
    .do(() => {
      this.ngZone.run(() => {
        this.idle.stop();
        this.appPass = '';
        this.appLogin = false;
        this.router.navigate(['/login'], {queryParams: {type: 'auth'}});
      });
    });

  public allContextMenu: ContextMenuComponent;
  public onlyCopyContextMenu: ContextMenuComponent;
  public pasteSelectContextMenu: ContextMenuComponent;

  constructor(private router: Router, private ngZone: NgZone, private contextMenuService: ContextMenuService) {
  }

  setExpMedTs(timestamp: number) {
    if (timestamp !== this.exp_med_ts) {
      this.exp_med_ts = timestamp;
      this.getExpMedTsEvent.next(timestamp);
    }
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

  changeAliases() {
    this.getAliasChangedEvent.next(true);
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
    this.idle.within(this.settings.appLockTime).start();
  }

  stopCountdown() {
    this.idle.stop();
  }

  restartCountdown() {
    this.idle.within(this.settings.appLockTime).restart();
  }

  public onContextMenu($event: MouseEvent): void {
    $event.target['contextSelectionStart'] = $event.target['selectionStart'];
    $event.target['contextSelectionEnd'] = $event.target['selectionEnd'];
    if ($event.target && ($event.target['nodeName'].toUpperCase() === 'TEXTAREA' || $event.target['nodeName'].toUpperCase() === 'INPUT') && !$event.target['readOnly']) {
      this.contextMenuService.show.next({
        contextMenu: this.allContextMenu,
        event: $event,
        item: $event.target,
      });
      $event.preventDefault();
      $event.stopPropagation();
    }
  }

  public onContextMenuOnlyCopy($event: MouseEvent, copyText?: string): void {
    this.contextMenuService.show.next({
      contextMenu: this.onlyCopyContextMenu,
      event: $event,
      item: copyText
    });
    $event.preventDefault();
    $event.stopPropagation();
  }

  public onContextMenuPasteSelect($event: MouseEvent): void {
    $event.target['contextSelectionStart'] = $event.target['selectionStart'];
    $event.target['contextSelectionEnd'] = $event.target['selectionEnd'];

    console.warn($event.target);
    console.warn($event.target['disabled']);


    if ($event.target && ($event.target['nodeName'].toUpperCase() === 'TEXTAREA' || $event.target['nodeName'].toUpperCase() === 'INPUT') && !$event.target['readOnly']) {
      this.contextMenuService.show.next({
        contextMenu: this.pasteSelectContextMenu,
        event: $event,
        item: $event.target,
      });
      $event.preventDefault();
      $event.stopPropagation();
    }
  }

}
