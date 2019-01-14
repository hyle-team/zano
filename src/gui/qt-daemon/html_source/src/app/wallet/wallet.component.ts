import {Component, OnInit, OnDestroy, NgZone} from '@angular/core';
import {ActivatedRoute, Router} from '@angular/router';
import {VariablesService} from '../_helpers/services/variables.service';
import {BackendService} from '../_helpers/services/backend.service';

@Component({
  selector: 'app-wallet',
  templateUrl: './wallet.component.html',
  styleUrls: ['./wallet.component.scss']
})
export class WalletComponent implements OnInit, OnDestroy {
  subRouting;
  walletID;
  tabs = [
    {
      title: 'WALLET.TABS.HISTORY',
      icon: 'history',
      link: '/history',
      indicator: false,
      active: true
    },
    {
      title: 'WALLET.TABS.SEND',
      icon: 'send',
      link: '/send',
      indicator: false,
      active: false
    },
    {
      title: 'WALLET.TABS.RECEIVE',
      icon: 'receive',
      link: '/receive',
      indicator: false,
      active: false
    },
    {
      title: 'WALLET.TABS.CONTRACTS',
      icon: 'contracts',
      link: '/contracts',
      indicator: 1,
      active: false
    },
    /*{
      title: 'WALLET.TABS.MESSAGES',
      icon: 'messages',
      link: '/messages',
      indicator: 32,
      active: false
    },*/
    {
      title: 'WALLET.TABS.STAKING',
      icon: 'staking',
      link: '/staking',
      indicator: false,
      active: false
    }
  ];

  constructor(
    private route: ActivatedRoute,
    private router: Router,
    private backend: BackendService,
    private variablesService: VariablesService,
    private ngZone: NgZone
  ) {
  }

  ngOnInit() {
    this.subRouting = this.route.params.subscribe(params => {
      this.walletID = +params['id'];
      this.variablesService.setCurrentWallet(this.walletID);
      for (let i = 0; i < this.tabs.length; i++) {
        this.tabs[i].active = (this.tabs[i].link === '/' + this.route.snapshot.firstChild.url[0].path);
      }
    });
  }

  changeTab(index) {
    if ((this.tabs[index].link === '/contracts' || this.tabs[index].link === '/staking') && this.variablesService.daemon_state !== 2) {
      return;
    }
    this.tabs.forEach((tab) => {
      tab.active = false;
    });
    this.tabs[index].active = true;
    this.router.navigate(['wallet/' + this.walletID + this.tabs[index].link]);
  }

  copyAddress() {
    this.backend.setClipboard(this.variablesService.currentWallet.address);
  }

  openInBrowser() {
    this.backend.openUrlInBrowser('zano.org');
  }

  ngOnDestroy() {
    this.subRouting.unsubscribe();
  }

}
