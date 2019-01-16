import {Component, OnInit, OnDestroy} from '@angular/core';
import {ActivatedRoute, Router} from '@angular/router';
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
    private router: Router,
    private variablesService: VariablesService
  ) {
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
