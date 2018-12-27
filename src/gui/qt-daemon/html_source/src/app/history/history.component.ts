import {Component, OnInit, OnDestroy} from '@angular/core';
import {ActivatedRoute} from '@angular/router';
import {BackendService} from '../_helpers/services/backend.service';
import {VariablesService} from '../_helpers/services/variables.service';

@Component({
  selector: 'app-history',
  templateUrl: './history.component.html',
  styleUrls: ['./history.component.scss']
})
export class HistoryComponent implements OnInit, OnDestroy {
  parentRouting;

  constructor(
    private route: ActivatedRoute,
    private backend: BackendService,
    private variablesService: VariablesService
  ) {
  }

  ngOnInit() {
    this.parentRouting = this.route.parent.params.subscribe(() => {
      console.log(this.variablesService.currentWallet.history);
    });
  }

  getHeight(item) {
    if ((this.variablesService.height_app - item.height >= 10 && item.height !== 0) || (item.is_mining === true && item.height === 0)) {
      return 100;
    } else {
      if (item.height === 0 || this.variablesService.height_app - item.height < 0) {
        return 0;
      } else {
        return (this.variablesService.height_app - item.height) * 10;
      }
    }
  }

  ngOnDestroy() {
    this.parentRouting.unsubscribe();
  }

}
