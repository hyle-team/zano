import {Component, OnInit, OnDestroy} from '@angular/core';
import {ActivatedRoute} from '@angular/router';
import {VariablesService} from '../_helpers/services/variables.service';

@Component({
  selector: 'app-contracts',
  templateUrl: './contracts.component.html',
  styleUrls: ['./contracts.component.scss']
})
export class ContractsComponent implements OnInit, OnDestroy {

  parentRouting;
  walletId;

  constructor(
    private route: ActivatedRoute,
    public variablesService: VariablesService
  ) {
  }

  public get sortedArrayContracts(): any[] {
    return this.variablesService.currentWallet.contracts.sort((a, b) => {
      if (a.is_new < b.is_new) {
        return 1;
      }
      if (a.is_new > b.is_new) {
        return -1;
      }
      if (a.timestamp < b.timestamp) {
        return 1;
      }
      if (a.timestamp > b.timestamp) {
        return -1;
      }
      return 0;
    });
  }

  ngOnInit() {
    this.parentRouting = this.route.parent.params.subscribe(params => {
      if (params.hasOwnProperty('id')) {
        this.walletId = params['id'];
      }
    });
  }

  ngOnDestroy() {
    this.parentRouting.unsubscribe();
  }

}
