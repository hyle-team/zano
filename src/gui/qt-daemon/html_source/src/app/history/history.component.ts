import {Component, OnInit, OnDestroy} from '@angular/core';
import {VariablesService} from '../_helpers/services/variables.service';

@Component({
  selector: 'app-history',
  templateUrl: './history.component.html',
  styleUrls: ['./history.component.scss']
})
export class HistoryComponent implements OnInit, OnDestroy {

  openedDetails = false;

  constructor(private variablesService: VariablesService) {}

  ngOnInit() {}

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

  openDetails(index) {
    if (index === this.openedDetails) {
      this.openedDetails = false;
    } else {
      this.openedDetails = index;
    }
  }

  ngOnDestroy() {}

}
