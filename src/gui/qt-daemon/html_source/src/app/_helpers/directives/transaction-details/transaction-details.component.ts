import {Component, OnInit, OnDestroy, Input} from '@angular/core';
import {Transaction} from '../../models/transaction.model';
import {VariablesService} from "../../services/variables.service";
import {BackendService} from '../../services/backend.service';

@Component({
  selector: 'app-transaction-details',
  templateUrl: './transaction-details.component.html',
  styleUrls: ['./transaction-details.component.scss']
})
export class TransactionDetailsComponent implements OnInit, OnDestroy {

  @Input() transaction: Transaction;
  @Input() sizes: Array<number>;

  constructor(private variablesService: VariablesService, private backendService: BackendService) {}

  ngOnInit() {}

  openInBrowser(tr) {
    let link = 'explorer.zano.org/transaction/' + tr;
    this.backendService.openUrlInBrowser(link);
  }

  ngOnDestroy() {}
}
